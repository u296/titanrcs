#ifndef RCS_UBO_H
#define RCS_UBO_H

#include "linalg.h"
#include "backend/backend.h"

typedef struct RcsUbo {
    Mat4 model;
    Mat4 view;
    Mat4 proj;
    Vec4 resolution_xy_L_;
} RcsUbo;

bool make_rcs_ubo(RenderBackend* rb, Buffer* ubo, CleanupStack* cs);

bool make_rcs_fftbuf(RenderBackend* rb, Buffer* rcs_fftbuf, CleanupStack* cs);

#endif