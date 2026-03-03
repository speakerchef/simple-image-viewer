#include "include/png.h"


RenderData *decode_png(FILE *file) {

    PNG_Metadata png_metadata = {0};
    // Process chunks
    const uint8_t CRC_SZ = 4;
    while (1) {
        uint32_t chunk_sz = 0;
        unsigned char *chunk_type = malloc(4);

        if (!fread(&chunk_sz, 1, 4, file)) break;
        if (!fread(chunk_type, 1, 4, file)) break;

        chunk_sz = ntohl(chunk_sz); // Big to little endian
        printf("Size of chunk is: %u, ", chunk_sz);
        printf("Chunk type is: %s\n", chunk_type);

        // Extract image size
        if (!memcmp(chunk_type, "IHDR", 4)) {
            /*
                *header is 13 bytes long, h,w stored in big endian
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
                case PNG_CS_GRAY: {
                    png_metadata.num_channels = 1;
                    break;
                }
                case PNG_CS_RGB: {
                    png_metadata.num_channels = 3;
                    break;
                }
                case PNG_CS_PLTE: {
                    png_metadata.num_channels = 1;
                    break;
                }
                case PNG_CS_GRAY_ALPHA: {
                    png_metadata.num_channels = 2;
                    break;
                }
                case PNG_CS_RGB_ALPHA: {
                    png_metadata.num_channels = 4;
                    break;
                }
            }

                printf("w:%hu h:%hu bd:%u cs:%u cm:%u fm:%u il:%u n_ch:%u\n",
                        png_metadata.width, png_metadata.height,
                        png_metadata.bit_depth, png_metadata.color_space,
                        png_metadata.compress_method, png_metadata.filter_method,
                        png_metadata.interlacing, png_metadata.num_channels);

                fseek(file, CRC_SZ, SEEK_CUR);
                continue;
            }

            // Extract color palette information if available
            if (!memcmp(chunk_type, "PLTE", 4)) {
                if (chunk_sz % 3 != 0) { fprintf(stderr, "Error: Invalid PLTE chunk.\n"); }

                png_metadata.palette = malloc(chunk_sz);
                fread(png_metadata.palette, 1, chunk_sz, file);

                fseek(file, CRC_SZ, SEEK_CUR);
                continue;
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

                fseek(file, CRC_SZ, SEEK_CUR);
                continue;
            }

            // End of file
            if (!memcmp(chunk_type, "IEND", 4)) {
                break;
            }

            fseek(file, chunk_sz + CRC_SZ, SEEK_CUR);
        }

    size_t scanline_size = (png_metadata.width * png_metadata.num_channels * png_metadata.bit_depth + 7) / 8;
    size_t output_size = png_metadata.height * (scanline_size + 1); // +1 to account for filter byte
    unsigned char *output_buffer = malloc(sizeof(unsigned char) * output_size);

    int ret = uncompress_png(png_metadata.image_data, 
                                output_buffer, 
                                png_metadata.total_size, 
                                output_size, 
                                &png_metadata);

    png_metadata.alpha_data = 0;
    ret = ret & load_png_colors(&png_metadata, png_metadata.alpha_data);

    load_png_colors(&png_metadata, png_metadata.alpha_data);

    printf("LOADED COLORS\n");
    
    RenderData *renderData = malloc(sizeof(RenderData));
    renderData->color = png_metadata.pixel_color;
    renderData->width = png_metadata.width;
    renderData->height = png_metadata.height;
    renderData->ret = ret;

    free(png_metadata.palette);
    free(png_metadata.image_data);
    free(png_metadata.ftype);

    return renderData;
    
}

int load_png_colors(PNG_Metadata *md, uint16_t alpha_data) {

    if (md->width < 1 || md->height < 1 || md->num_channels < 1){
        fprintf(stderr, "Error: Could not not load colors; Invalid metadata!\n");
        return 1;
    }

    md->pixel_color = malloc(sizeof(SDL_Color) * md->width * md->height);
    md->ftype = malloc(sizeof(unsigned char) * md->height);
    size_t scanline_width = (md->width * md->num_channels * md->bytes_per_channel) + 1;
    uint16_t r, g, b, a;

    // Handle color loading for all color spaces 
    switch (md->color_space) {
        case PNG_CS_PLTE: {
            for (size_t y = 0; y < md->height; y++) {
                for (size_t x = 0; x < md->width; x++) {
                    uint16_t index = md->image_data[y * scanline_width + x + 1];
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

        // if (md->color_space == PNG_CS_RGB || md->color_space == CS_RGB_ALPHA) {
        case PNG_CS_RGB: {

            printf("Loading PNG_CS_RGB Colors\n");

            size_t stride = md->width * md->num_channels * md->bytes_per_channel;
            unsigned char *unfiltered = malloc(sizeof(unsigned char) * md->height * stride);
            for (size_t y = 0; y < md->height; y++) {

                // Extract filter type
                md->ftype[y] = md->image_data[y * scanline_width];

                int ret;
                if ((ret = unfilter_png(md->ftype[y], y, unfiltered, scanline_width, stride, md)) != 0) {
                    return ret; 
                }

                for (size_t x = 0; x < md->width; x++) {
                    size_t offset = (y * stride) + (x * md->num_channels * md->bytes_per_channel);

                    r = unfiltered[offset];
                    g = unfiltered[offset + 2];
                    b = unfiltered[offset + 4];
                    // a = alpha_data ? alpha_data : 255;
                    a = 255;

                    md->pixel_color[y * md->width + x] = (SDL_Color){r, g, b, a};
                }
            }
            break;
        }
    }

    return 0;
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

int __filter_sub(PNG_Metadata *md, unsigned char *unfiltered, const size_t row_idx){
    /*
     * To remove the Sub filter, we will add the previous pixel
     * to the current pixel to reconstruct the current pixel.
     * It is always the case that the prev pixel of first pixel 
     * in every scanline is 0.
     * */
    if (md->width < 1 || md->height < 1 || md->num_channels < 1){
        fprintf(stderr, "Error: Could not apply filter; Invalid metadata!\n");
        return 1;
    }

    size_t scanline_width = md->width * md->num_channels * md->bytes_per_channel + 1;
    size_t stride = md->width * md->num_channels * md->bytes_per_channel;
    size_t decrement_sz = md->num_channels * md->bytes_per_channel;

    for (size_t x = 0; x < stride; x++) {
        size_t offset = row_idx * scanline_width + x + 1;
        uint32_t prev = (x >= decrement_sz) ? unfiltered[row_idx * stride + (x - decrement_sz)] 
                                            : 0;
       
        unfiltered[row_idx * stride + x] = md->image_data[offset] + prev;
    }

    return 0;
}

int __filter_up(PNG_Metadata *md, unsigned char *unfiltered, const size_t row_idx) {
    /* 
     * To remove the Up filter, we will 
     * add the previous pixel at the previous
     * row to the current pixel to reconstruct the 
     * current pixel. It is always the case 
     * that the prev pixel of the first pixel 
     * in every scanline is 0.
     */
    if (md->width < 1 || md->height < 1 || md->num_channels < 1){
        fprintf(stderr, "Error: Could not apply filter; Invalid metadata!\n");
        return 1;
    }

    size_t scanline_width = md->width * md->num_channels * md->bytes_per_channel + 1;
    size_t stride = md->width * md->num_channels * md->bytes_per_channel;

    for (size_t x = 0; x < stride; x++) {

        size_t offset = row_idx * scanline_width + x + 1;
        uint32_t prev = row_idx ? unfiltered[(row_idx - 1) * stride + x] 
                            : 0;

        unfiltered[row_idx * stride + x] = md->image_data[offset] + prev;
    }

    return 0;
}

int __filter_avg(PNG_Metadata *md, unsigned char *unfiltered, const size_t row_idx) {
    /*
     * This filter combines both the up and sub filters.
     * We desconstruct by */
    if (md->width < 1 || md->height < 1 || md->num_channels < 1){
        fprintf(stderr, "Error: Could not apply filter; Invalid metadata!\n");
        return 1;
    }

    size_t scanline_width = md->width * md->num_channels * md->bytes_per_channel + 1;
    size_t stride = md->width * md->num_channels * md->bytes_per_channel;
    size_t decrement_sz = md->num_channels * md->bytes_per_channel;

    for (size_t x = 0; x < stride; x++) {

        size_t offset = row_idx * scanline_width + x + 1;
        uint32_t prev_a = (x >= decrement_sz) ? unfiltered[row_idx * stride + (x - decrement_sz)] 
                                            : 0;
        uint32_t prev_b = row_idx ? unfiltered[(row_idx - 1) * stride + x] 
                            : 0;

        unfiltered[row_idx * stride + x] = md->image_data[offset] + ((prev_a + prev_b) >> 1);
    }

    return 0;
}
int __filter_paeth(PNG_Metadata *md, unsigned char *unfiltered, const size_t row_idx) {

    if (md->width < 1 || md->height < 1 || md->num_channels < 1){
        fprintf(stderr, "Error: Could not apply filter; Invalid metadata!\n");
        return 1;
    }

    size_t scanline_width = md->width * md->num_channels * md->bytes_per_channel + 1;
    size_t stride = md->width * md->num_channels * md->bytes_per_channel;
    size_t decrement_sz = md->num_channels * md->bytes_per_channel;

    for (size_t x = 0; x < stride; x++) {
        /*
         * using paeth predictor as shown at https://www.w3.org/TR/png-3/#paethpredictor-function 
         * */
        size_t offset = row_idx * scanline_width + x + 1;
        uint32_t prev_a = (x >= decrement_sz) ? unfiltered[row_idx * stride + (x - decrement_sz)] 
                                            : 0;
        uint32_t prev_b = row_idx ? unfiltered[(row_idx - 1) * stride + x] 
                            : 0;
        uint32_t prev_c = (x >= decrement_sz) ? unfiltered[(row_idx - 1) * stride + (x - decrement_sz)]
                                            : 0;

        unfiltered[row_idx * stride + x] = md->image_data[offset] + ((prev_a + prev_b) >> 1);
    }

    return 0;
}

int unfilter_png(const unsigned char ftype, 
                 const size_t row_idx, 
                 unsigned char *unfiltered, 
                 const size_t scanline_width,
                 const size_t stride,
                 PNG_Metadata *md) {
    
    switch (ftype) {
        case PNG_FILTER_NONE: { 
            size_t offset = row_idx * scanline_width + 1;
            memcpy(unfiltered + row_idx * stride, md->image_data + offset, stride);
            break;
        };
        case PNG_FILTER_SUB: {
            __filter_sub(md, unfiltered, row_idx);
            break;
        }
        case PNG_FILTER_UP: {
            __filter_up(md, unfiltered, row_idx);
            break;
        }
        case PNG_FILTER_AVG: {
            __filter_avg(md, unfiltered, row_idx);
            break;
        }
        case PNG_FILTER_PAETH: {
            __filter_paeth(md, unfiltered, row_idx);
            break;
        }
    }

    return 0;
}

