#ifndef RES_H
#define RES_H

#include "common.h"

#ifdef __APPLE__
constexpr u32 RCS_RESOLUTION = 4096u;
#else
constexpr u32 RCS_RESOLUTION = 8192u;
#endif
constexpr bool WANT_VALIDATION = true;
constexpr u32 N_MAX_INFLIGHT = 1;

#endif