/*
 * Author: Sohan Nair
 * Date:
 *
 * */
#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "utils.h"

#define FILE_STRIDE 2
#define DHT_LEN 4

typedef enum {
    // Huffman coding types
    HUFFMAN_BASELINE_DCT         = 0xFFC0,
    HUFFMAN_EXTENDED_SEQ_DCT     = 0xFFC1,
    HUFFMAN_PROGRESSIVE_DCT      = 0xFFC2,
    HUFFMAN_LOSSLESS             = 0xFFC3,
    HUFFMAN_DIFF_SEQ_DCT         = 0xFFC5,
    HUFFMAN_DIFF_PROGRESSIVE_DCT = 0xFFC6,
    HUFFMAN_DIFF_LOSSLESS_DCT    = 0xFFC7,

    HUFFMAN_DHT                  = 0xFFC4,
    JPEG_DQT                     = 0xFFDB,
    JPEG_DRI                     = 0xFFDD,
    JPEG_DNL                     = 0xFFDC,
    JPEG_SOS                     = 0xFFDA, // Signals compressed data
    JPEG_EOI                     = 0xFFD9,

    // Restart intervals
    JPEG_RST1                    = 0xFFD0,
    JPEG_RST2                    = 0xFFD1,
    JPEG_RST3                    = 0xFFD2,
    JPEG_RST4                    = 0xFFD3,
    JPEG_RST5                    = 0xFFD4,
    JPEG_RST6                    = 0xFFD5,
    JPEG_RST7                    = 0xFFD6,
    JPEG_RST8                    = 0xFFD7,


} JPEG_HEADER_MARKER;

typedef struct {
    JPEG_HEADER_MARKER huffman_coding_type;
    unsigned char APP0_ident[5]; // "JFIF"
    ColorData8 *color_data;
    unsigned char *image_data;
    unsigned char *huffman_coding_data;
    unsigned char *dht_table[DHT_LEN];
    unsigned char *huffman_table;
    unsigned char *qtable_data;
    unsigned char *restart_interval;
    uint16_t y_dens;
    uint16_t x_dens;
    uint16_t APP0_len;
    unsigned char APP0_Marker[2];
    unsigned char JFIF_Vers[2];
    unsigned char density_unit;
    unsigned char thumbnail_w;
    unsigned char thumbnail_h;
} JPEG_Metadata;


int decode_jpeg(FILE *file, size_t file_sz);

