#pragma once

#include "global_def.h"

std::string ToSecondStr(long nano, const char* format="%Y-%m-%d %H:%M:%S");

long NanoTime();

std::string NanoTimeStr();
