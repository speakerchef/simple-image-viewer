#pragma once
#include <stdint.h>
#include <SDL3/SDL.h>

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

typedef struct ColorData {
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t a;
} ColorData;

typedef struct RenderData {
    ColorData *color;
    ColorData bg_color; 
    
    uint32_t width;
    uint32_t height;
    uint8_t bytes_per_channel;
    uint8_t num_channels;
    bool set_bg;
    int ret;
} RenderData;

