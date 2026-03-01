#include <SDL3/SDL.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct PNG_Metadata {
    uint64_t width;
    uint64_t height;
    uint64_t bit_depth;
    uint64_t color_space;
    uint64_t compress_method;
    uint64_t filter_method;
    uint64_t interlacing;
    unsigned char *image_data;
} PNG_Metadata;

static PNG_Metadata png_metadata;

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Error: Invalid arguments; expected file-path "
               "'path/to/image\n");

        return 0;
    }

    const char *path = argv[1];
    FILE *file = fopen(path, "rb");

    if (!file) {
        fprintf(stderr, "Could not open file.\n");
        return 1;
    }

    png_metadata.image_data = NULL;


    // Get file length
    fseek(file, 0, SEEK_END);
    size_t file_len = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Get PNG file identifier (first 8 bytes of the file)
    char identifier[8] = {0}; 
    fread(identifier, 1, 8, file);
    unsigned char *byteid = (unsigned char*)identifier;
    // for (size_t i = 0; i < 8; i++) {
    //     printf("%02x", byteid[i]);
    // }
    // printf("\n");

    unsigned char __png_id[8] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a}; // PNG Spec ID
    bool is_PNG = !(memcmp(byteid, __png_id, 8));
    // printf("%d\n", is_PNG);

    fseek(file, 8, SEEK_SET); // Skip past ID to data chunks

    while(1) {
        unsigned long chunk_sz = 0;
        char chunk_type[4]; 

        if (fread(&chunk_sz, 1, 4, file) == 0) break;
        chunk_sz = ntohl(chunk_sz); //Big to little endian
        printf("Size of chunk is: %ld, ", chunk_sz);

        if (fread(chunk_type, 1, 4, file) == 0) break;
        printf("Chunk type is: ");
        for (size_t i = 0; i < 4; i++) {
            printf("%c", chunk_type[i]);
        }
        printf("\n");

        // Extract image size
        if (!memcmp(chunk_type, "IHDR", 4)) {
            /*
             *header is 13 bytes long, stored in big endian
             *00 00 00 00 | 00 00 00 00 | 00 | 00 | 00 | 00 | 00
             *     w             h        BD   CS   CM   FM   IL
             * */ 
            uint64_t __width;
            uint64_t __height;
            unsigned char __bit_depth;
            unsigned char __color_space;
            unsigned char __compress_method;
            unsigned char __filter_method;
            unsigned char __interlacing;

            fread(&__width, 1, 4, file);
            fread(&__height, 1, 4, file);
            fread(&__bit_depth, 1, 1, file);
            fread(&__color_space, 1, 1, file);
            fread(&__compress_method, 1, 1, file);
            fread(&__filter_method, 1, 1, file);
            fread(&__interlacing, 1, 1, file);

            png_metadata.width = ntohl(__width);
            png_metadata.height = ntohl(__height);
            png_metadata.bit_depth = (uint64_t)__bit_depth;
            png_metadata.color_space = (uint64_t)__color_space;
            png_metadata.compress_method = (uint64_t)__compress_method;
            png_metadata.filter_method = (uint64_t)__filter_method;
            png_metadata.interlacing = (uint64_t)__interlacing;

            printf("%llu %llu %llu %llu %llu %llu %llu\n", 
                   png_metadata.width,
                   png_metadata.height,
                   png_metadata.bit_depth,
                   png_metadata.color_space, 
                   png_metadata.compress_method, 
                   png_metadata.filter_method, 
                   png_metadata.interlacing
                   );

            fseek(file, -chunk_sz, SEEK_CUR);
        }

        // Extract image data
        size_t total_size = 0;
        if (!memcmp(chunk_type, "IDAT", 4)) {
            char img_data[chunk_sz];
            fread(img_data, 1, chunk_sz, file);
            unsigned char *img_data_bytes = (unsigned char*)img_data;

            // Dynamically reallocate if png contains multiple IDAT chunks
            png_metadata.image_data = realloc(png_metadata.image_data, total_size + chunk_sz);
            total_size += chunk_sz;

            png_metadata.image_data = img_data_bytes;

            if (memcmp(png_metadata.image_data, img_data_bytes, chunk_sz) != 0) {
                fprintf(stderr, "Unable to load PNG data.\n");
                return 1;
            }
        }

        if (!memcmp(chunk_type, "IEND", 4)) {
            break;
        }

        fseek(file, chunk_sz + 4, SEEK_CUR);
    }
      
    
    fclose(file);

    uint8_t x = png_metadata.width;
    uint8_t y = png_metadata.height;

    SDL_Window *window = SDL_CreateWindow("Image Viewer", x, y, 0);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED,
                          SDL_WINDOWPOS_CENTERED);
    SDL_CreateRenderer(window, NULL);

    if (window) {

        SDL_Event event;
        bool isRunning = true;

        // App loop
        while (isRunning) {
            // printf("Is running: %d\n", isRunning);

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

            // Rendering pipeline
            SDL_Renderer *renderer = SDL_GetRenderer(window);

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            // SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            //
            // Do pixel rendering here.
            //
            SDL_RenderPresent(renderer); // Display

            // TODO: Explore render destructor and other safety/ semantic
            // practies defined in docs!!

            // isRunning = false;
        }

        return 0;
    }

    printf("Error opening window.\n");
}
