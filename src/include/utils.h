#pragma once
#include <stdint.h>
#include <SDL3/SDL.h>

typedef struct RenderData {
    SDL_Color *color;
    
    uint32_t width;
    uint32_t height;
    uint8_t bytes_per_channel;
    uint8_t num_channels;
    int ret;
} RenderData;

