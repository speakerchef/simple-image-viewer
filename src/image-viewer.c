#include "include/png.h"
#include "include/jpeg.h"
#include "include/utils.h"
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <SDL3/SDL_render.h>
#include <string.h>

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
    unsigned char png_identifier[8] = {0};
    unsigned char jpeg_identifier[2] = {0};
    // if (!fread(png_identifier, 1, 8, file)) {
    //     fprintf(stderr, ERR_BAD_FILE); 
    //     return 1;
    // }

    fread(png_identifier, 1, 8, file);
    fseek(file, 0, SEEK_SET);
    fread(jpeg_identifier, 1, 2, file);
    bool is_PNG = !(memcmp(png_identifier, _png_id, 8));
    bool is_JPEG = !(memcmp(jpeg_identifier, _jpeg_id, 2));
    // printf("Is PNG: %d, is JPEG: %d\n", is_PNG, is_JPEG);

    printf("Identifier: ");
    for (size_t i = 0; i < 2; i++) {
        printf("%02X ", jpeg_identifier[i]);
    }
    printf("\n");


    RenderData *renderData = NULL;

    if (is_PNG) {
        fseek(file, 8, SEEK_SET); // Skip past ID
        renderData = decode_png(file); 
    }
    if (is_JPEG) {
        int res = decode_jpeg(file, file_sz); 
    }


    // if (!renderData) {
    //     fprintf(stderr, ERR_BAD_FILE);
    //     return 1;
    // }

    fclose(file);

    if (!renderData->color || !renderData->height || !renderData->width) {
        fprintf(stderr, ERR_BAD_FILE);
        return 1;
    }


    int x = renderData->width;
    int y = renderData->height;
    
    SDL_Window *window = SDL_CreateWindow("Image Viewer", x, y, 0);

    if (window) {

        SDL_SetWindowResizable(window, true);
        SDL_DisplayID *display_id = SDL_GetDisplays(NULL);
        const SDL_DisplayMode *display_mode = SDL_GetCurrentDisplayMode(display_id[0]);
        SDL_free(display_id);

        if (renderData->width >= MAX_RENDER_RATIO * display_mode->w || 
            renderData->height >= MAX_RENDER_RATIO * display_mode->h) {

            float scale = MIN(MAX_RENDER_RATIO * display_mode->h / y, MAX_RENDER_RATIO * display_mode->w / x);

            x = floor(x * scale);
            y = floor(y * scale);

        } 

        SDL_SetWindowSize(window, x, y);

        SDL_SetWindowPosition(window,
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED
                              );
        bool isRunning = true;
        SDL_Event event;
        int pixel_format = SDL_PIXELFORMAT_RGBA64;
        int pitch = renderData->width * 8;
        const float aspect_ratio = (double)renderData->width / renderData->height;

        SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
        SDL_Surface *surface = SDL_CreateSurfaceFrom(renderData->width, renderData->height, pixel_format, renderData->color, pitch);

        /*
         * Ambiguous sRGB and other sources
         * already have gamma encoded in 
         * unlike my raw HDR pipeline
         */
        if (renderData->is_hdr) {
            SDL_SetSurfaceColorspace(surface, SDL_COLORSPACE_SRGB_LINEAR);
        }

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

        const int min_w = MIN_SCREEN_SIZE_RATIO * display_mode->w;
        const int min_h = MIN_SCREEN_SIZE_RATIO * display_mode->h;
        SDL_SetWindowMinimumSize(window, min_w, min_h);
        
        // Maintain image aspect ratio
        // resize.
        SDL_SetRenderLogicalPresentation(renderer, x, y, SDL_LOGICAL_PRESENTATION_LETTERBOX);

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
            int cur_x, cur_y = 0;
            SDL_GetWindowSize(window, &cur_x, &cur_y);
            

            if (renderData->set_bg) {
                SDL_SetRenderDrawColor(renderer, 
                                       renderData->bg_color.r, 
                                       renderData->bg_color.g, 
                                       renderData->bg_color.b, 
                                       renderData->bg_color.a);
            }
            SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, texture, NULL, NULL);
            SDL_SetRenderVSync(renderer, 60);
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

