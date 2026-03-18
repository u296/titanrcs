#include "linalg.h"

#include <stdio.h>

Mat4 transpose_m4(Mat4 A) {
    Mat4 T = {
        {
            A.v[0], A.v[4], A.v[8], A.v[12],
            A.v[1], A.v[5], A.v[9], A.v[13],
            A.v[2], A.v[6], A.v[10], A.v[14],
            A.v[3], A.v[7], A.v[11], A.v[15]
        }
    };
    return T;
}

Mat4 add_m4(Mat4 A, Mat4 B) {
    Mat4 C = {
        {
            A.v[0] + B.v[0], A.v[1] + B.v[1], A.v[2] + B.v[2], A.v[3] + B.v[3],
            A.v[4] + B.v[4], A.v[5] + B.v[5], A.v[6] + B.v[6], A.v[7] + B.v[7],
            A.v[8] + B.v[8], A.v[9] + B.v[9], A.v[10] + B.v[10], A.v[11] + B.v[11],
            A.v[12] + B.v[12], A.v[13] + B.v[13], A.v[14] + B.v[14], A.v[15] + B.v[15],
        }
    };
    return C;
}

Mat4 muls_m4(f32 a, Mat4 A) {
    Mat4 B = {
        {
            a*A.v[0], a*A.v[1], a*A.v[2], a*A.v[3],
            a*A.v[4], a*A.v[5], a*A.v[6],  a*A.v[7],
            a*A.v[8], a*A.v[9], a*A.v[10], a*A.v[11],
            a*A.v[12], a*A.v[13], a*A.v[14], a*A.v[15]
        }
    };

    return B;
}

f32 index_m4(Mat4 A, u32 row, u32 col) {
    return A.v[col * 4 + row];
}

f32* pindex_m4(Mat4* A, u32 row, u32 col) {
    return &A->v[col * 4 + row];
}

Mat4 mul_m4(Mat4 A, Mat4 B) {
    Mat4 C = {};

    //C[row i column j] = A[row i] dot B[column j]

    for (u32 i = 0; i < 4; i++) {
        for (u32 j = 0; j < 4; j++) {

            f32* target = pindex_m4(&C, i, j);
            f32 scalarprod = 0.0f;
            // compute the scalar product
            for (u32 k = 0; k < 4; k++) {
                scalarprod += index_m4(A,i,k) * index_m4(B, k, j);
            }
            *target = scalarprod;
        }
    }

    return C;
}

void print_m4(Mat4 A) {
    printf("[%2.2f %2.2f %2.2f %2.2f]\n", index_m4(A, 0, 0), index_m4(A, 0, 1), index_m4(A, 0, 2), index_m4(A, 0, 3));
    printf("[%2.2f %2.2f %2.2f %2.2f]\n", index_m4(A, 1, 0), index_m4(A, 1, 1), index_m4(A, 1, 2), index_m4(A, 1, 3));
    printf("[%2.2f %2.2f %2.2f %2.2f]\n", index_m4(A, 2, 0), index_m4(A, 2, 1), index_m4(A, 2, 2), index_m4(A, 2, 3));
    printf("[%2.2f %2.2f %2.2f %2.2f]\n", index_m4(A, 3, 0), index_m4(A, 3, 1), index_m4(A, 3, 2), index_m4(A, 3, 3));
}