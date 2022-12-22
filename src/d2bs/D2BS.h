#pragma once

#ifndef XP_WIN
#define XP_WIN
#endif

#include "d2bs/core/ScreenHook.h"
#include "d2bs/variables.h"
#include "d2bs/version.h"

#include <queue>
#include <vector>
#include <windows.h>

BOOL Startup(void);
void Shutdown(void);
