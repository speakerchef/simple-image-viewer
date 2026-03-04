#include "include/png.h"
#include "include/utils.h"
#include <stdint.h>
#include <stdio.h>
#include <SDL3/SDL_render.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Error: Invalid arguments; expected file-path 'path/to/image'\n");

        return 1;
    }
    const char *path = argv[1];
    FILE *file = fopen(path, "rb");

    if (!file) {
        fprintf(stderr, "Could not open file.\n");
        return 1;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    size_t file_sz = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Get PNG file identifier (first 8 bytes of the file)
    unsigned char identifier[8] = {0};
    fread(identifier, 1, 8, file);

    unsigned char __png_id[8] = {0x89, 0x50, 0x4e, 0x47,
                                 0x0d, 0x0a, 0x1a, 0x0a}; // PNG Spec ID

    bool is_PNG = !(memcmp(identifier, __png_id, 8));

    fseek(file, 8, SEEK_SET); // Skip past ID to data chunks

    RenderData *renderData = NULL; 

    if (is_PNG) { renderData = decode_png(file); }

    fclose(file);

    if (!renderData->color || !renderData->height || !renderData->width) {
        fprintf(stderr, "Error: Could not open image; File data corrupt or invalid.\n");
        return 1;
    }

    printf("RENDER DATA IS: w:%u  h:%u \n", renderData->width, renderData->height);

    uint32_t x = renderData->width;
    uint32_t y = renderData->height;

    SDL_Window *window = SDL_CreateWindow("Image Viewer", x, y, 0);
    SDL_SetWindowPosition(window, 
                          SDL_WINDOWPOS_CENTERED,
                          SDL_WINDOWPOS_CENTERED
                        );

    if (window) {

        bool isRunning = true;
        SDL_Event event;
        int pitch = renderData->width * renderData->num_channels * 2;
        printf("Pitch is: %u\n", pitch);
        SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
        SDL_Surface *surface = SDL_CreateSurfaceFrom(x, y, SDL_PIXELFORMAT_RGBA32, renderData->color, x * 4);
        if (surface == NULL) {
            SDL_Log("CreateRGBSurface failed: %s", SDL_GetError());
            exit(1);
        }

        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

        if (texture == NULL) {
            SDL_Log("CreateRGBSurface failed: %s", SDL_GetError());
            exit(1);
        }

        SDL_DestroySurface(surface);
        surface = NULL;
        // printf("\x1b[38;2;255;100;0mThis is true color text\x1b[0m\n");
        // TODO: Add custom terminal based rendering


         while (isRunning) {

             // Event handling
             while (SDL_PollEvent(&event)) {
                 switch (event.type) {
                 case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                     isRunning = false;
                     break;
                 case SDL_EVENT_KEY_DOWN:
                     isRunning = !(event.key.scancode == 20);
                     break;
                 }
             }

             // SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
             SDL_RenderClear(renderer);
             SDL_RenderTexture(renderer, texture, NULL, NULL);
             SDL_SetRenderVSync(renderer, 1);
             SDL_RenderPresent(renderer);


         }

         free(renderData);
         SDL_DestroyTexture(texture);
         SDL_DestroyRenderer(renderer);
         SDL_Quit();
        
        return 0;
    }


    printf("Error opening window.\n");
}





            // // Rendering pipeline
            // for (size_t dy = 0; dy < y; dy++) {
            //     for (size_t dx = 0; dx < x; dx++) {
            //
            //         uint16_t r = renderData->color[dy * x + dx].r;
            //         uint16_t g = renderData->color[dy * x + dx].g;
            //         uint16_t b = renderData->color[dy * x + dx].b;
            //         uint16_t a = renderData->color[dy * x + dx].a;
            //         // printf("%u %u %u %u", r, g, b, a);
            //
            //         SDL_SetRenderDrawColor(renderer, r, g, b, 0);
            //
            //         SDL_RenderPoint(renderer, dx, dy);
            //
            //     }
            // }
            // SDL_RenderPresent(renderer);
