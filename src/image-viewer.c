#include "include/png.h"
#include "include/utils.h"
#include <stddef.h>
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
        fprintf(stderr, ERR_BAD_FILE);
        return 1;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    size_t file_sz = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Get PNG file identifier (first 8 bytes of the file)
    unsigned char identifier[8] = {0};
    if (!fread(identifier, 1, 8, file)) {
        fprintf(stderr, ERR_BAD_FILE); 
        return 1;
    }

    unsigned char __png_id[8] = {0x89, 0x50, 0x4e, 0x47,
                                 0x0d, 0x0a, 0x1a, 0x0a}; // PNG Spec ID

    bool is_PNG = !(memcmp(identifier, __png_id, 8));

    fseek(file, 8, SEEK_SET); // Skip past ID to data chunks

    RenderData *renderData = NULL;

    if (is_PNG) { renderData = decode_png(file); }

    if (!renderData) {
        fprintf(stderr, ERR_BAD_FILE);
        return 1;
    }

    fclose(file);

    if (!renderData->color || !renderData->height || !renderData->width) {
        fprintf(stderr, ERR_BAD_FILE);
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
        int pixel_format = SDL_PIXELFORMAT_RGBA64;
        int pitch = x * 8;

        SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
        SDL_Surface *surface = SDL_CreateSurfaceFrom(x, y, pixel_format, renderData->color, pitch);
        SDL_SetSurfaceColorspace(surface, SDL_COLORSPACE_SRGB_LINEAR);

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
        free(renderData->color);
        surface = NULL;


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


            if (renderData->set_bg) {
                SDL_SetRenderDrawColor(renderer, 
                                       renderData->bg_color.r, 
                                       renderData->bg_color.g, 
                                       renderData->bg_color.b, 
                                       renderData->bg_color.a);
            }
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

    // while (1) { sleep(5000);};
    printf("Error opening window.\n");
}

