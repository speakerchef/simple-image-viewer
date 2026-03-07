#pragma once
#include <stdint.h>
#include <SDL3/SDL.h>

#define ERR_BAD_FILE "Error: Could not open image; File data corrupt or invalid.\n" 

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

