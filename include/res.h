#ifndef RES_H
#define RES_H

#include "common.h"

#ifdef __APPLE__
constexpr u64 RCS_RESOLUTION = 4096ull;
#else
constexpr u64 RCS_RESOLUTION = 8192ull;
#endif
constexpr u64 RCS_CROPFRACTION =
    4; // if ~2 or less, output will be bugged near the edges
constexpr f32 RCS_RANGE = 1000000.0f;
constexpr f32 RCS_BOXSIZE = 20.0f;
constexpr f32 RCS_LINEWIDTH = 4;
/*
Setting the linewidth too high will cause part of the line to get hidden behind
the geometry, this needs to be taken into consideration.

I just tried linewidths 1, 3, 4 and 32 and they all pretty much worked the same.
They gave the same RCS, although 1 had a lot of bullshit in the far field. 32
started to lag behind a little on the big peaks, but other than that they're
identical, even down to the noise they get. Just be sure to keep the ribbon
smaller than the wavelength.

Low linewidths looks really scary because shit flies across the screen all of the
time but I checked again, it doesn't affect the output
*/
constexpr bool WANT_VALIDATION = true;
constexpr bool BUILD_SHARP_EDGES = true;
constexpr u32 N_MAX_INFLIGHT = 1;
constexpr u32 FFT_IMG_TEXELSIZE =
    sizeof(float) * 4; // fft image is R32G32B32A32_SFLOAT

#endif