#ifndef LINALG_H
#define LINALG_H
#include "common.h"

typedef struct Vec2 {
    float x;
    float y;
} Vec2;

typedef struct __attribute__((packed)) Vec3 {
    float x;
    float y;
    float z;
} Vec3;

typedef struct Vec4 {
    float x;
    float y;
    float z;
    float w;
} Vec4;

typedef struct Mat3 {
    f32 v[3*3];
} Mat3;

// column major
typedef struct Mat4 {
    f32 v[4 * 4];
} Mat4;

Vec3 sub_v3(Vec3 a, Vec3 b);
Vec3 add_v3(Vec3 a, Vec3 b);
f32 dot_v3(Vec3 a, Vec3 b);
Vec3 cross_v3(Vec3 a, Vec3 b);
Vec3 muls_v3(f32 a, Vec3 b);
f32 len_v3(Vec3 a);
Vec3 normalize_v3(Vec3 a);

Mat3 transpose_m3(Mat3 A);
Mat3 invert_m3(Mat3 A);
Mat3 subm4_m3(Mat4 A);
void print_m3(Mat3 A);

Mat4 zeroed_from_m3(Mat3 A);
Mat4 transpose_m4(Mat4 A);
f32 index_m4(Mat4 A, u32 row, u32 col);
f32* pindex_m4(Mat4* A, u32 row, u32 col);
Mat4 mul_m4(Mat4 A, Mat4 B);
Mat4 muls_m4(f32 a, Mat4 A);
Mat4 add_m4(Mat4 A, Mat4 B);
void print_m4(Mat4 A);

#endif