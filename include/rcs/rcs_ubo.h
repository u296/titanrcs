#ifndef RCS_UBO_H
#define RCS_UBO_H

#include "backend/backend.h"
#include "linalg.h"

typedef struct RcsUbo {
    Mat4 model;
    Mat4 view;
    Mat4 proj;
    Mat4 norm_trans;
    Vec4 resolution_xy_L_lambda;
} RcsUbo;

typedef struct ExtractionSsbo {
    f32 out_intensity; // square of the E field, NOT compensated with eta0
} ExtractionSsbo;

bool make_rcs_ubo(RenderBackend* rb, Buffer* ubo, CleanupStack* cs);

bool make_rcs_fftbuf(RenderBackend* rb, Buffer* rcs_fftbuf, CleanupStack* cs);

bool make_rcs_fftimg(RenderBackend* rb, VkExtent2D ext, Image* rcs_fftimg,
                     CleanupStack* cs);

bool make_rcs_extrbuf(RenderBackend* rb, Buffer* rcs_extrbuf, CleanupStack* cs);

#endif