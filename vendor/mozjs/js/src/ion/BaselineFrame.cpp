/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=99:
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "BaselineFrame.h"
#include "BaselineFrame-inl.h"
#include "BaselineIC.h"
#include "BaselineJIT.h"
#include "Ion.h"
#include "IonFrames-inl.h"

#include "vm/Debugger.h"
#include "vm/ScopeObject.h"

using namespace js;
using namespace js::ion;

void
BaselineFrame::trace(JSTracer *trc)
{
    MarkCalleeToken(trc, calleeToken());

    gc::MarkValueRoot(trc, &thisValue(), "baseline-this");

    // Mark actual and formal args.
    if (isNonEvalFunctionFrame()) {
        unsigned numArgs = js::Max(numActualArgs(), numFormalArgs());
        JS_ASSERT(actuals() == formals());
        gc::MarkValueRootRange(trc, numArgs, actuals(), "baseline-args");
    }

    // Mark scope chain.
    gc::MarkObjectRoot(trc, &scopeChain_, "baseline-scopechain");

    // Mark return value.
    if (hasReturnValue())
        gc::MarkValueRoot(trc, returnValue(), "baseline-rval");

    if (isEvalFrame())
        gc::MarkScriptRoot(trc, &evalScript_, "baseline-evalscript");

    if (hasArgsObj())
        gc::MarkObjectRoot(trc, &argsObj_, "baseline-args-obj");

    // Mark locals and stack values.
    size_t nvalues = numValueSlots();
    if (nvalues > 0) {
        // The stack grows down, so start at the last Value.
        Value *last = valueSlot(nvalues - 1);
        gc::MarkValueRootRange(trc, nvalues, last, "baseline-stack");
    }
}

bool
BaselineFrame::copyRawFrameSlots(AutoValueVector *vec) const
{
    unsigned nfixed = script()->nfixed;
    unsigned nformals = numFormalArgs();

    if (!vec->resize(nformals + nfixed))
        return false;

    PodCopy(vec->begin(), formals(), nformals);
    for (unsigned i = 0; i < nfixed; i++)
        (*vec)[nformals + i] = *valueSlot(i);
    return true;
}

bool
BaselineFrame::strictEvalPrologue(JSContext *cx)
{
    JS_ASSERT(isStrictEvalFrame());

    CallObject *callobj = CallObject::createForStrictEval(cx, this);
    if (!callobj)
        return false;

    pushOnScopeChain(*callobj);
    flags_ |= HAS_CALL_OBJ;
    return true;
}

bool
BaselineFrame::heavyweightFunPrologue(JSContext *cx)
{
    return initFunctionScopeObjects(cx);
}

bool
BaselineFrame::initFunctionScopeObjects(JSContext *cx)
{
    JS_ASSERT(isNonEvalFunctionFrame());
    JS_ASSERT(fun()->isHeavyweight());

    CallObject *callobj = CallObject::createForFunction(cx, this);
    if (!callobj)
        return false;

    pushOnScopeChain(*callobj);
    flags_ |= HAS_CALL_OBJ;
    return true;
}

bool
BaselineFrame::initForOsr(StackFrame *fp, uint32_t numStackValues)
{
    PodZero(this);

    scopeChain_ = fp->scopeChain();

    if (fp->hasCallObjUnchecked())
        flags_ |= BaselineFrame::HAS_CALL_OBJ;

    if (fp->hasBlockChain()) {
        flags_ |= BaselineFrame::HAS_BLOCKCHAIN;
        blockChain_ = &fp->blockChain();
    }

    if (fp->isEvalFrame()) {
        flags_ |= BaselineFrame::EVAL;
        evalScript_ = fp->script();
    }

    if (fp->script()->needsArgsObj() && fp->hasArgsObj()) {
        flags_ |= BaselineFrame::HAS_ARGS_OBJ;
        argsObj_ = &fp->argsObj();
    }

    if (fp->hasHookData()) {
        flags_ |= BaselineFrame::HAS_HOOK_DATA;
        hookData_ = fp->hookData();
    }

    if (fp->hasPushedSPSFrame())
        flags_ |= BaselineFrame::HAS_PUSHED_SPS_FRAME;

    frameSize_ = BaselineFrame::FramePointerOffset +
        BaselineFrame::Size() +
        numStackValues * sizeof(Value);

    JS_ASSERT(numValueSlots() == numStackValues);

    for (uint32_t i = 0; i < numStackValues; i++)
        *valueSlot(i) = fp->slots()[i];

    JSContext *cx = GetIonContext()->cx;
    if (cx->compartment->debugMode()) {
        // In debug mode, update any Debugger.Frame objects for the StackFrame to
        // point to the BaselineFrame.

        // The caller pushed a fake return address. ScriptFrameIter, used by the
        // debugger, wants a valid return address, but it's okay to just pick one.
        // In debug mode there's always at least 1 ICEntry (since there are always
        // debug prologue/epilogue calls).
        IonFrameIterator iter(cx->mainThread().ionTop);
        JS_ASSERT(iter.returnAddress() == NULL);
        BaselineScript *baseline = fp->script()->baseline;
        iter.current()->setReturnAddress(baseline->returnAddressForIC(baseline->icEntry(0)));

        if (!Debugger::handleBaselineOsr(cx, fp, this))
            return false;
    }

    return true;
}
