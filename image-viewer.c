#include <SDL3/SDL.h>
#include <assert.h>
#include <limits.h>
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
    uint64_t bytes_per_channel;
    uint64_t alpha_data;
    unsigned char bit_depth;
    unsigned char color_space;
    unsigned char compress_method;
    unsigned char filter_method;
    unsigned char interlacing;
    uint64_t pixel_size;
    uint64_t num_channels;
    unsigned char *image_data;
    SDL_Color *pixel_color;
    unsigned char *palette;
    size_t total_size;
} PNG_Metadata;

// Fwd decs
void load_png_colors(PNG_Metadata *md, uint32_t alpha_data);
int uncompress_png(unsigned char *input, 
                   unsigned char *output, 
                   const size_t in_buf_size, 
                   const size_t out_buf_size, 
                   PNG_Metadata *md);
;
void load_png_colors(PNG_Metadata *md, uint32_t alpha_data) {

    md->pixel_color = malloc(sizeof(SDL_Color) * md->width * md->height);
    size_t scanline_width = (md->width * md->num_channels * md->bytes_per_channel) + 1;
    uint8_t r, g, b, a;

    // Handle color loading for all color spaces 
    switch (md->color_space) {
        case CS_PLTE: {
            for (size_t y = 0; y < md->height; y++) {
                for (size_t x = 0; x < md->width; x++) {
                    uint8_t index = md->image_data[y * scanline_width + x + 1];
                    uint16_t stride = index * 3;

                    r = md->palette[stride];
                    g = md->palette[stride + 1];
                    b = md->palette[stride + 2];
                    a = 255;

                    md->pixel_color[y * md->width + x] = (SDL_Color){r, g, b, a};
                }
            }
            break;
        }

        // if (md->color_space == CS_RGB || md->color_space == CS_RGB_ALPHA) {
        case CS_RGB: {
            printf("Loading CS_RGB Colors\n");
            for (size_t y = 0; y < md->height; y++) {
                for (size_t x = 0; x < md->width; x++) {
                    size_t offset = y * scanline_width + 1 + (x * md->num_channels * md->bytes_per_channel);

                    // TODO: implement filtering
                    r = md->image_data[offset];
                    g = md->image_data[offset + 2];
                    b = md->image_data[offset + 4];
                    // a = alpha_data ? alpha_data : 255;
                    a = 255;

                    md->pixel_color[y * md->width + x] = (SDL_Color){r, g, b, a};
                }
            }
            break;
        }
    }
}

int uncompress_png(unsigned char *in_buf, 
                   unsigned char *out_buf, 
                   const size_t in_buf_size, 
                   const size_t out_buf_size,
                   PNG_Metadata *md) {

    z_stream stream;
    int level = PNG_COMPRESSION_LEVEL;
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
    stream.avail_in = in_buf_size;
    stream.avail_out = out_buf_size;
    stream.next_in = in_buf;
    stream.next_out = out_buf;

    // https://zlib.net/zlib_how.html
    ret = inflate(&stream, Z_NO_FLUSH);
    assert(ret != Z_STREAM_ERROR);

    switch (ret) {
    case Z_NEED_DICT:
        ret = Z_DATA_ERROR; 
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
        (void)inflateEnd(&stream);
        return ret;
    }

    (void)inflateEnd(&stream);
    
    md->image_data = out_buf;

    return 0;
}

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

    // Get file size
    fseek(file, 0, SEEK_END);
    size_t file_sz = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Get PNG file identifier (first 8 bytes of the file)
    char identifier[8] = {0};
    fread(identifier, 1, 8, file);
    unsigned char *byteid = (unsigned char *)identifier;

    unsigned char __png_id[8] = {0x89, 0x50, 0x4e, 0x47,
                                 0x0d, 0x0a, 0x1a, 0x0a}; // PNG Spec ID
    bool is_PNG = !(memcmp(byteid, __png_id, 8));
    printf("IS PNG?: %d\n", is_PNG);

    fseek(file, 8, SEEK_SET); // Skip past ID to data chunks

    static PNG_Metadata png_metadata;

    if (is_PNG) {

        png_metadata.image_data = NULL;
        png_metadata.palette = NULL;
        png_metadata.total_size = 0;

        // Process chunks
        while (1) {
            unsigned long chunk_sz = 0;
            char chunk_type[4];

            if (!fread(&chunk_sz, 1, 4, file)) break;
            if (!fread(chunk_type, 1, 4, file)) break;

            chunk_sz = ntohl(chunk_sz); // Big to little endian
            printf("Size of chunk is: %ld, ", chunk_sz);

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
                fread(&png_metadata.width, 1, 4, file);
                fread(&png_metadata.height, 1, 4, file);
                fread(&png_metadata.bit_depth, 1, 1, file);
                fread(&png_metadata.color_space, 1, 1, file);
                fread(&png_metadata.compress_method, 1, 1, file);
                fread(&png_metadata.filter_method, 1, 1, file);
                fread(&png_metadata.interlacing, 1, 1, file);

                png_metadata.width = ntohl(png_metadata.width);
                png_metadata.height = ntohl(png_metadata.height);
                png_metadata.bytes_per_channel = png_metadata.bit_depth / 8;

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

                printf("w:%llu h:%llu bd:%u cs:%u cm:%u fm:%u intl:%u n_ch:%llu\n",
                       png_metadata.width, png_metadata.height,
                       png_metadata.bit_depth, png_metadata.color_space,
                       png_metadata.compress_method, png_metadata.filter_method,
                       png_metadata.interlacing, png_metadata.num_channels);

                fseek(file, -chunk_sz, SEEK_CUR);
            }

            // Extract color palette information if available
            if (!memcmp(chunk_type, "PLTE", 4)) {
                png_metadata.palette = malloc(chunk_sz);
                fread(png_metadata.palette, 1, chunk_sz, file);

                fseek(file, -chunk_sz, SEEK_CUR);
            }

            // Extract image data and append sequential IDAT chunks
            if (!memcmp(chunk_type, "IDAT", 4)) {

                png_metadata.image_data = 
                    realloc(png_metadata.image_data, 
                            png_metadata.total_size + chunk_sz
                );

                fread(png_metadata.image_data + png_metadata.total_size, 
                      1, 
                      chunk_sz, 
                      file
                );

                png_metadata.total_size += chunk_sz;

                fseek(file, -chunk_sz, SEEK_CUR);
            }

            // End of file
            if (!memcmp(chunk_type, "IEND", 4)) {
                break;
            }

            fseek(file, chunk_sz + 4, SEEK_CUR);
        }

        size_t scanline_size = (png_metadata.width * png_metadata.num_channels * png_metadata.bit_depth + 7) / 8;
        size_t output_size = png_metadata.height * (scanline_size + 1); // +1 to account for filter byte
        unsigned char *output_buffer = malloc(sizeof(unsigned char) * output_size);

        uncompress_png(png_metadata.image_data, output_buffer, png_metadata.total_size, output_size, &png_metadata);
        printf("Data uncompressed\n");

        png_metadata.alpha_data = 0;
        load_png_colors(&png_metadata, png_metadata.alpha_data);

        printf("LOADED COLORS\n");

    }

    fclose(file);

    uint32_t x = png_metadata.width;
    uint32_t y = png_metadata.height;

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

            // FIX: slow ass rendering -> turn into texture
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
                    // printf("%u %u %u %u", r, g, b, a);

                    SDL_SetRenderDrawColor(renderer, r, g, b, a);
                    // SDL_SetRenderDrawColor(renderer, 255, 124, 111, 255);

                    SDL_RenderPoint(renderer, dx, dy);

                }
            }
            SDL_RenderPresent(renderer);

            // TODO: Explore render destructor and other safety/ semantic
            // practies defined in docs!!
        }

        free(png_metadata.image_data);
        free(png_metadata.pixel_color);
        return 0;
    }


    printf("Error opening window.\n");
}
