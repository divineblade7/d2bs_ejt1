#pragma once

#include "d2bs/variables.h"

#include <Windows.h>

class CriticalRoom {
 public:
  CriticalRoom() {
    EnterSection();
  }

  ~CriticalRoom() {
    LeaveSection();
  }

  void release() {
    LeaveSection();
  }

  void release_for(int milliseconds) const {
    LeaveSection();
    Sleep(milliseconds);
    EnterSection();
  }

 private:
  void EnterSection() const {
    InterlockedIncrement(&Vars.SectionCount);
    bEnteredCriticalSection = true;
    EnterCriticalSection(&Vars.cGameLoopSection);
    InterlockedDecrement(&Vars.SectionCount);
  }

  void LeaveSection() const {
    if (bEnteredCriticalSection) {
      bEnteredCriticalSection = false;
      LeaveCriticalSection(&Vars.cGameLoopSection);
    }
  }

 private:
  mutable bool bEnteredCriticalSection = false;
};
