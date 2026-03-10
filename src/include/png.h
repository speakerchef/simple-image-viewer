/*
 * Author: Sohan Nair
 * Date:
 *
 * */
#pragma once

#include <stdint.h>
#include <assert.h>
#include <limits.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>
#include "utils.h"

#define PNG_COMPRESSION_LEVEL 8

// PNG color space types
#define PNG_CS_GRAY 0
#define PNG_CS_RGB 2
#define PNG_CS_PLTE 3
#define PNG_CS_GRAY_ALPHA 4
#define PNG_CS_RGB_ALPHA 6

// sRGB types
#define SRGB_PERCEPT 0
#define SRGB_REL_COLOR 1
#define SRGB_SATURATION 2
#define SRGB_ABS_COLOR 3


// PNG filter types
#define PNG_FILTER_NONE 0
#define PNG_FILTER_SUB 1
#define PNG_FILTER_UP 2
#define PNG_FILTER_AVG 3
#define PNG_FILTER_PAETH 4

#define REC_709_GAMMA 2.4
#define HLG_REF_GAMMA 1.2
// #define BT2100_REF_WHITE 0.0203
#define BT2100_REF_WHITE 0.01

typedef struct ColorData ColorData;

typedef struct PNG_Metadata {
    ColorData8 bg_color; 
    unsigned char *image_data;
    unsigned char *palette;
    unsigned char *transparency;
    ColorData16 *pixel_color;
    size_t total_size;
    size_t trns_sz;
    uint32_t width;
    uint32_t height;
    uint16_t alpha_data;
    uint16_t bytes_per_channel;
    uint16_t pixel_size;
    uint16_t num_channels;
    unsigned char bit_depth;
    unsigned char color_space;
    unsigned char compress_method;
    unsigned char filter_method; // Useless
    unsigned char interlacing;
    bool set_bg;
    bool is_plt;
    bool is_gray;
    bool is_alpha;
    bool is_hdr;
    bool is_pq;
    bool is_hlg;
    bool has_iccp;
    bool is_srgb;
    int8_t srgb_type;
} PNG_Metadata;

// Calculated using BT 2020 and Rec. 709
// primaries. Specifically: T = tranpose(inv(709) * 2100)
// with white point D65
// https://physics.stackexchange.com/questions/487763/how-are-the-matrices-for-the-rgb-to-from-cie-xyz-conversions-generated#:~:text=In%20the%20end%2C%20it's%20just,)%20=%201%20%2D%20x%20%2D%20y
static double _xyz2srgb_d65_const_mat[9] = {
    1.660376, -0.124417, -0.018110,
    -0.587656,  1.132856, -0.100556,
    -0.072850, -0.008381,  1.118788
};

static const Matrix xyz2rgb_mat = {_xyz2srgb_d65_const_mat, 3, 3};

// Fwd decs
void _set_color(uint16_t *unfiltered, const size_t stride, PNG_Metadata *md, const size_t y);
void apply_R709_gamma(double *I);
void apply_sRGB_gamma(double *I);
double pq_transfer_func(uint16_t *E_pr);
void _hlg_OOTF(double *E, const double *Y_s);
double hlg_transfer_func(uint16_t *E_pr);
int load_png_colors(PNG_Metadata *md, uint16_t alpha_data);
int uncompress_png(unsigned char *input, 
                   unsigned char *output, 
                   const size_t in_buf_size, 
                   const size_t out_buf_size, 
                   PNG_Metadata *md);
int unfilter_png(const unsigned char ftype, 
                 const size_t row_idx, 
                 uint16_t *unfiltered, 
                 const size_t scanline_width, 
                 const size_t stride,
                 PNG_Metadata *md);

int __filter_sub(PNG_Metadata *md,  uint16_t *unfiltered, const size_t row_idx);
int __filter_up(PNG_Metadata *md,   uint16_t *unfiltered, const size_t row_idx);
int __filter_avg(PNG_Metadata *md,  uint16_t *unfiltered, const size_t row_idx); 
int __filter_paeth(PNG_Metadata *md,uint16_t *unfiltered, const size_t row_idx); 
RenderData *decode_png(FILE *file);

