#include <SDL3/SDL.h>
#include <limits.h>
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

#define PNG_COMPRESSION_LEVEL 8

#define CS_GRAY 0
#define CS_RGB 2
#define CS_PLTE 3
#define CS_GRAY_ALPHA 4
#define CS_RGB_ALPHA 6

typedef struct PNG_Metadata {
    uint64_t width;
    uint64_t height;
    uint64_t bit_depth;
    uint64_t color_space;
    uint64_t compress_method;
    uint64_t filter_method;
    uint64_t interlacing;
    uint64_t pixel_size;
    uint64_t num_channels;
    unsigned char *image_data;
    SDL_Color *pixel_color;
    unsigned char *palette;
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
    
    #define WINDOW_SIZE file_len

    // Get PNG file identifier (first 8 bytes of the file)
    char identifier[8] = {0}; 
    fread(identifier, 1, 8, file);
    unsigned char *byteid = (unsigned char*)identifier;

    unsigned char __png_id[8] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a}; // PNG Spec ID
    bool is_PNG = !(memcmp(byteid, __png_id, 8));

    fseek(file, 8, SEEK_SET); // Skip past ID to data chunks

    // Process chunks
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

            // Set color space information
            switch (png_metadata.color_space) {
                case CS_GRAY: {
                    png_metadata.num_channels = 1;
                    break;
                }
                case CS_RGB: {
                    png_metadata.num_channels = 3;
                    break;
                }
                case CS_PLTE: {
                    png_metadata.num_channels = 1;
                    break;
                }
                case CS_GRAY_ALPHA: {
                    png_metadata.num_channels = 2;
                    break;
                }
                case CS_RGB_ALPHA: {
                    png_metadata.num_channels = 4;
                    break;
                }
            }

            printf("%llu %llu %llu %llu %llu %llu %llu %llu\n", 
                   png_metadata.width,
                   png_metadata.height,
                   png_metadata.bit_depth,
                   png_metadata.color_space, 
                   png_metadata.compress_method, 
                   png_metadata.filter_method, 
                   png_metadata.interlacing,
                   png_metadata.num_channels
                   );

            fseek(file, -chunk_sz, SEEK_CUR);
        }

        // Extract color palette/alpha information if available
        if (!memcmp(chunk_type, "PLTE", 4)) {
            png_metadata.palette = malloc(256 * 3);
            char palette[256 * 3];

            fread(palette, 1, chunk_sz, file);
            unsigned char *palette_bytes = (unsigned char*)palette;
            png_metadata.palette = palette_bytes;

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

            z_stream stream; int level = PNG_COMPRESSION_LEVEL;
            stream.data_type = Z_BINARY;
            stream.avail_in = 0;
            stream.next_in = Z_NULL;
            stream.zalloc = Z_NULL;
            stream.opaque = Z_NULL;
            stream.zfree = Z_NULL;

            int ret = inflateInit(&stream);
            if (ret != Z_OK) {
                return ret;
            }

            // Note: no filtering pipeline yet
            size_t scanline_size = (png_metadata.width * png_metadata.bit_depth + 7) / 8;
            size_t output_size = png_metadata.num_channels * png_metadata.height * (scanline_size + 1); // +1 to account for filter byte
            stream.avail_in = total_size;
            stream.avail_out = output_size;
            stream.next_in = img_data_bytes;
            stream.next_out = png_metadata.image_data;

            // https://zlib.net/zlib_how.html
            ret = inflate(&stream, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&stream);
                return ret;
            }
            (void)inflateEnd(&stream);

        }

        // End of file
        if (!memcmp(chunk_type, "IEND", 4)) {
            break;
        }

        fseek(file, chunk_sz + 4, SEEK_CUR);
    }

    // Get pixel colors
    png_metadata.pixel_color = malloc(sizeof(SDL_Color) * png_metadata.width * png_metadata.height); 
    size_t scanline_width = png_metadata.width + 1; 

    for (size_t y = 0; y < png_metadata.height; y++) {
        for (size_t x = 0; x < png_metadata.width; x++) {

            uint8_t index = png_metadata.image_data[y * scanline_width + x + 1];
            uint16_t stride = index * 3;
            uint8_t r = png_metadata.palette[stride];
            uint8_t g = png_metadata.palette[stride + 1];
            uint8_t b = png_metadata.palette[stride + 2];

            png_metadata.pixel_color[y * png_metadata.width + x] = (SDL_Color){r, g, b, 255};

        }
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


            SDL_Renderer *renderer = SDL_GetRenderer(window);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            // Rendering pipeline
            for (size_t dy = 0; dy < y; dy++) {
                for (size_t dx = 0; dx < x; dx++) {

                    uint8_t r = png_metadata.pixel_color[dy * png_metadata.width + dx].r;
                    uint8_t g = png_metadata.pixel_color[dy * png_metadata.width + dx].g;
                    uint8_t b = png_metadata.pixel_color[dy * png_metadata.width + dx].b;
                    uint8_t a = png_metadata.pixel_color[dy * png_metadata.width + dx].a;

                    SDL_SetRenderDrawColor(renderer, r, g, b, a);

                    SDL_RenderPoint(renderer, dx, dy);


                }
            } 

            SDL_RenderPresent(renderer);

            // TODO: Explore render destructor and other safety/ semantic
            // practies defined in docs!!
        }

        return 0;
    }

    printf("Error opening window.\n");
}
