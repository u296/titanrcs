#ifndef RCS_UBO_H
#define RCS_UBO_H

#include "linalg.h"
#include "backend/backend.h"

typedef struct RcsUbo {
    Mat4 model;
    Mat4 view;
    Mat4 proj;
} RcsUbo;

bool make_rcs_ubo(RenderBackend* rb, Buffer* ubo, CleanupStack* cs);

#endif