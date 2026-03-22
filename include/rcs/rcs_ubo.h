#ifndef RCS_UBO_H
#define RCS_UBO_H

#include "linalg.h"

typedef struct RcsUbo {
    Mat4 model;
    Mat4 view;
    Mat4 proj;
} RcsUbo;

#endif