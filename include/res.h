#ifndef RES_H
#define RES_H

#include "common.h"

#ifdef __APPLE__
constexpr u64 RCS_RESOLUTION = 4096ull;
#else
constexpr u64 RCS_RESOLUTION = 4096ull;
#endif
constexpr u64 RCS_CROPFRACTION = 16; // if ~2 or less, output will be bugged near the edges
constexpr f32 RCS_RANGE = 1000000.0f;
constexpr f32 RCS_BOXSIZE = 20.0f;
constexpr bool WANT_VALIDATION = true;
constexpr u32 N_MAX_INFLIGHT = 1;
constexpr u32 FFT_IMG_TEXELSIZE = sizeof(float) * 4; // fft image is R32G32B32A32_SFLOAT

#endif