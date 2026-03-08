#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <assert.h>


#define ERR_BAD_FILE "Error: Could not open image; File data corrupt or invalid.\n" 

/*
 * Perceptual quantization (PQ) constants
 * As defined in Rec. ITU-R BT.2100-3
 * */
#define _M1 ( 0.1593017578125 )
#define _M2 ( 78.84375 )
#define _C2 ( 18.8515625 )
#define _C3 ( 18.6875 )
#define _C1 ( (_C3 - _C2) + 1 )

// Utils
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define CLAMP(a, max, min) ({     \
    if ((a) > (max)) (a) = (max); \
    if ((a) < (min)) (a) = (min); \
})

typedef struct ColorDataF {
    float r;
    float g;
    float b;
    float a;
} ColorDataF;

typedef struct ColorData16 {
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t a;
} ColorData16;

typedef struct ColorData8 {
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t a;
} ColorData8;

typedef struct RenderData {
    ColorData16 *color;
    ColorData8 bg_color; 
    
    uint32_t width;
    uint32_t height;
    uint8_t bytes_per_channel;
    uint8_t num_channels;
    bool set_bg;
    int ret;
} RenderData;

typedef struct Matrix {
    double *coeffs;
    size_t rows;
    size_t cols;
} Matrix;

void _matrix_mult(const Matrix *A, const Matrix *B, Matrix *C); 
