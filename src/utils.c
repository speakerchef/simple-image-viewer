#include "include/utils.h"

void _matrix_mult(const Matrix *A, const Matrix *B, Matrix *C) {

    assert(A->cols == B->rows);

    for (size_t z = 0; z < A->rows; z++) {
        for (size_t x = 0; x < B->cols; x++) {
            double sum = 0;
            for (size_t y = 0; y < B->rows; y++) {
                sum += A->coeffs[z * A->cols + y] * B->coeffs[y * B->cols + x];
            }
            C->coeffs[z * C->cols + x] = sum;
        }
    }
}

