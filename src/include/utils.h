#pragma once
#include <stdint.h>
#include <SDL3/SDL.h>

typedef struct RenderData {
    SDL_Color *color;
    
    uint32_t width;
    uint32_t height;
    int ret;
} RenderData;

