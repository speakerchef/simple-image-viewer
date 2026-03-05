#pragma once
#include <stdint.h>
#include <SDL3/SDL.h>

typedef struct ColorData {
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t a;
} ColorData;

typedef struct RenderData {
    ColorData *color;
    
    uint32_t width;
    uint32_t height;
    uint8_t bytes_per_channel;
    uint8_t num_channels;
    int ret;
} RenderData;

