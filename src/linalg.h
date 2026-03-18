#ifndef LINALG_H
#define LINALG_H
#include "common.h"

// column major
typedef struct Mat4 {
    f32 v[4*4];
} Mat4;


Mat4 transpose_m4(Mat4 A);
f32 index_m4(Mat4 A, u32 row, u32 col);
f32* pindex_m4(Mat4* A, u32 row, u32 col);
Mat4 mul_m4(Mat4 A, Mat4 B);
Mat4 muls_m4(f32 a, Mat4 A);
Mat4 add_m4(Mat4 A, Mat4 B);
void print_m4(Mat4 A);

#endif