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

// PNG filter types
#define PNG_FILTER_NONE 0
#define PNG_FILTER_SUB 1
#define PNG_FILTER_UP 2
#define PNG_FILTER_AVG 3
#define PNG_FILTER_PAETH 4


typedef struct ColorData ColorData;

typedef struct PNG_Metadata {
    ColorData bg_color; 
    unsigned char *image_data;
    unsigned char *palette;
    unsigned char *transparency;
    ColorData *pixel_color;
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
} PNG_Metadata;


// Fwd decs
void _set_color(uint16_t *unfiltered, const size_t stride, PNG_Metadata *md, const size_t y, bool is_gray, bool is_plt, bool is_alpha);
void _prc_pq_transfer_func(uint16_t *E_pr);
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

