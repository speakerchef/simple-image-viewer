#include "include/png.h"
#include "include/utils.h"
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: Handle cICP chunks
//  - TODO: Implement Perceptual quantization (PQ) transfer function
// TODO: Error handling
// TODO: Adam7 interlacing
// TODO: Support 1, 2, 4 bit depth color spaces
// TODO: Add dynamic quant based on monitor specs
// TODO: Gamma gAMA chunk handling

RenderData *decode_png(FILE *file) {

    PNG_Metadata png_metadata = {0};

    // Process chunks
    const uint8_t CRC_SZ = 4;
    while (1) {
        uint32_t chunk_sz = 0;
        unsigned char chunk_type[4] = {0};

        if (!fread(&chunk_sz, 1, 4, file))
            break;
        if (!fread(chunk_type, 1, 4, file))
            break;

        chunk_sz = ntohl(chunk_sz); // Big to little endian
        printf("Size of chunk is: %u, ", chunk_sz);

        printf("Chunk type is: ");
        for (int i = 0; i < 4; i++) {
            printf("%c", chunk_type[i]);
        }
        printf(" \n");

        // Extract image size
        if (!memcmp(chunk_type, "IHDR", 4)) {
            /*
             * Read in metadata, note h,w,
             * */

            size_t ret = 0;
            ret += fread(&png_metadata.width, 1, 4, file);
            ret += fread(&png_metadata.height, 1, 4, file);
            ret += fread(&png_metadata.bit_depth, 1, 1, file);
            ret += fread(&png_metadata.color_space, 1, 1, file);
            ret += fread(&png_metadata.compress_method, 1, 1, file);
            ret += fread(&png_metadata.filter_method, 1, 1, file);
            ret += fread(&png_metadata.interlacing, 1, 1, file);

            if (ret != 13) {
                return NULL;
            }

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

        if (!memcmp(chunk_type, "cICP", 4)) {
            unsigned char cicp_dat[chunk_sz]; 
            fread(cicp_dat, sizeof(char), chunk_sz, file);

            for (size_t i = 0; i < chunk_sz; i++) {
                printf("%u ", cicp_dat[i]); 
            }
            printf("\n");


            fseek(file, CRC_SZ, SEEK_CUR);
            continue;
        }

        // Extract color palette information if available
        if (!memcmp(chunk_type, "PLTE", 4)) {
            if (chunk_sz % 3 != 0) {
                fprintf(stderr, "Error: Invalid PLTE chunk.\n");
            }

            png_metadata.palette = calloc(chunk_sz, sizeof(char));
            fread(png_metadata.palette, sizeof(char), chunk_sz, file);

            fseek(file, CRC_SZ, SEEK_CUR);
            continue;
        }

        if (!memcmp(chunk_type, "tRNS", 4)) {
            uint8_t span = PNG_CS_PLTE ? 1 : 2; // PLTE tRNS chunk has 1 bytes sequences 
            png_metadata.transparency = calloc(1, chunk_sz);

            fread(png_metadata.transparency, sizeof(char), chunk_sz, file);
            png_metadata.trns_sz = chunk_sz;

            fseek(file, CRC_SZ, SEEK_CUR);
            continue;
        }

        // Get and set background colors if defined
        if (!memcmp(chunk_type, "bKGD", 4)) {
            if (png_metadata.color_space == PNG_CS_GRAY || png_metadata.color_space == PNG_CS_GRAY_ALPHA) {
                unsigned char bkgd_dat[chunk_sz];
                fread(bkgd_dat, sizeof(char), chunk_sz, file);
                png_metadata.set_bg = 1;

                png_metadata.bg_color.r = ( bkgd_dat[0] << 8 ) | bkgd_dat[1];
                png_metadata.bg_color.a = UINT16_MAX;

            } else if (png_metadata.color_space == PNG_CS_RGB || png_metadata.color_space == PNG_CS_RGB_ALPHA) {
                unsigned char bkgd_dat[chunk_sz];
                fread(bkgd_dat, sizeof(char), chunk_sz, file);
                png_metadata.set_bg = 1;

                png_metadata.bg_color.r = ( bkgd_dat[0] << 8 ) | bkgd_dat[1];
                png_metadata.bg_color.g = ( bkgd_dat[2] << 8 ) | bkgd_dat[3];
                png_metadata.bg_color.b = ( bkgd_dat[4] << 8 ) | bkgd_dat[5];
                png_metadata.bg_color.a = UINT16_MAX;

            } else {
                unsigned char bkgd_dat;
                fread(&bkgd_dat, sizeof(char), chunk_sz, file);
                png_metadata.bg_color.r = png_metadata.palette[bkgd_dat];
                png_metadata.bg_color.g = png_metadata.palette[bkgd_dat + 1];
                png_metadata.bg_color.b = png_metadata.palette[bkgd_dat + 2];
                png_metadata.bg_color.a = UINT8_MAX;
                // printf("r: %u, g: %u, b: %u \n", png_metadata.bg_color.r, png_metadata.bg_color.g, png_metadata.bg_color.b);
            }


            fseek(file, CRC_SZ, SEEK_CUR);
            continue;
        }

        // Extract image data and append sequential IDAT chunks if (!memcmp(chunk_type, "IDAT", 4)) {
            png_metadata.image_data = realloc(
                png_metadata.image_data, png_metadata.total_size + chunk_sz);

            fread(png_metadata.image_data + png_metadata.total_size, 1,
                  chunk_sz, file);

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

    size_t scanline_size = (png_metadata.width * png_metadata.num_channels *
                                png_metadata.bit_depth +
                            7) /
                           8;
    size_t output_size = png_metadata.height *
                         (scanline_size + 1); // +1 to account for filter byte
    unsigned char *output_buffer = calloc(output_size, sizeof(unsigned char));

    int ret =
        uncompress_png(png_metadata.image_data, output_buffer,
                       png_metadata.total_size, output_size, &png_metadata);

    png_metadata.alpha_data = 0;
    ret = ret & load_png_colors(&png_metadata, png_metadata.alpha_data);

    printf("LOADED COLORS\n");

    RenderData *renderData = calloc(1, sizeof(RenderData));
    renderData->color = png_metadata.pixel_color;
    renderData->width = png_metadata.width;
    renderData->height = png_metadata.height;
    renderData->bytes_per_channel = png_metadata.bytes_per_channel;
    renderData->num_channels = png_metadata.num_channels;
    renderData->bg_color = png_metadata.bg_color;
    renderData->set_bg = png_metadata.set_bg;
    renderData->ret = ret;

    free(png_metadata.palette);
    free(png_metadata.image_data);

    return renderData;
}

void _set_color(uint16_t *unfiltered, 
                const size_t stride, 
                PNG_Metadata *md,
                const size_t y, 
                bool is_gray, 
                bool is_plt, 
                bool is_alpha
                ) {

    uint16_t r, g, b, a;
    uint16_t desired_bdepth = 8;
    bool is_quant = 0;

    for (size_t x = 0; x < md->width; x++) {

        size_t offset = (y * stride) + (x * md->num_channels * md->bytes_per_channel);
        uint16_t index = unfiltered[offset];
        uint16_t palette_stride = index * 3;


        if (md->bytes_per_channel > 1) {
            // TRUE 16 bit color
            r = (unfiltered[offset] << desired_bdepth) | unfiltered[offset + 1];
            g = (unfiltered[offset + 2] << desired_bdepth) | unfiltered[offset + 3];
            b = (unfiltered[offset + 4] << desired_bdepth) | unfiltered[offset + 5];
            a = is_alpha ? (unfiltered[offset + 6] << desired_bdepth) | unfiltered[offset + 7]
                         : UINT16_MAX;

            // printf("R BEFORE: %u ", r);
            // _prc_pq_transfer_func(&r);
            // printf("R AFTER: %u \n", r);

            if (is_quant) {
                r = r >> desired_bdepth;
                g = g >> desired_bdepth;
                b = b >> desired_bdepth;
            }
        } else { // <= 8bit
            if (is_plt) {
                r = md->palette[palette_stride];
                g = md->palette[palette_stride + 1];
                b = md->palette[palette_stride + 2];
                a = ( index < md->trns_sz ) ? md->transparency[index]
                                            : UINT8_MAX;
            } else {
                r = unfiltered[offset];
                g = unfiltered[offset + 1];
                b = unfiltered[offset + 2];
                a = is_alpha ? unfiltered[offset + 3]
                             : UINT8_MAX;
            }

            // Scale 8bit to 16bit
            r = (r << 8);
            g = (g << 8);
            b = (b << 8);
            a = (a << 8);
        } // <= 8bit

        if (is_gray) {
            md->pixel_color[y * md->width + x] = (ColorData){r, r, r, a};
        } else {
            md->pixel_color[y * md->width + x] = (ColorData){r, g, b, a};
        }
    }
}

void _prc_pq_transfer_func(uint16_t *E_pr) {
    /*
     * Non-linear to linear space
     * As defined in Rec. ITU-R BT.2100-3
     * */
    const double input = (double)*E_pr / UINT16_MAX;
    const double m2_const = (1 / _M2);
    const double m1_const = (1 / _M1);

    //BOOM
    double _max = MAX( (pow(input, m2_const) - _C1), 0 );
    double _dc3 =  _C3 * pow(input, m2_const);
    double _b = fabs(_max / ( _C2 - _dc3 ));
    double luminance = pow(_b, m1_const);
    
    const double matrix[3][3] = {
        { 3.2404542, -1.5371385, -0.4985314 },
        { -0.9692660,  1.8760108,  0.0415560 },
        { 0.0556434, -0.2040259,  1.0572252 }
    };

    double chrom_r[2] = {0.708, 0.292};
    double chrom_g[2] = {0.170, 0.797};
    double chrom_b[2] = {0.131, 0.046};
    double ref_wht[2] = {0.3127, 0.3290};
    // printf("Y: %lf\n", y);

    // uint16_t res = y * UINT16_MAX;
    // printf("RES: %u\n", res);
    // *E_pr = (uint16_t)floor(10000 * y);
}

int load_png_colors(PNG_Metadata *md, uint16_t alpha_data) {

    if (md->width < 1 || md->height < 1 || md->num_channels < 1) {
        fprintf(stderr, "Error: Could not not load colors; Invalid metadata!\n");
        return 1;
    }

    md->pixel_color = calloc(md->width * md->height, sizeof(ColorData));
    size_t stride = md->width * md->num_channels * md->bytes_per_channel;
    size_t scanline_width = stride + 1;
    uint16_t r, g, b, a;
    uint16_t *unfiltered = calloc(md->height * stride, sizeof(uint16_t));
    bool is_plt, is_gray, is_alpha = 0;


    switch (md->color_space) {

        case PNG_CS_GRAY: {
            is_gray = 1;

            for (size_t y = 0; y < md->height; y++) {
                unfilter_png(md->image_data[y * scanline_width], // Filter type
                            y, unfiltered, scanline_width, stride, md);

                _set_color(unfiltered, stride, md, y, is_gray, is_plt, is_alpha);
            }

            free(unfiltered);
            break;
        }

        case PNG_CS_GRAY_ALPHA: {
            is_gray = 1;
            is_alpha = 1;

            for (size_t y = 0; y < md->height; y++) {
                unfilter_png(md->image_data[y * scanline_width], // Filter type
                            y, unfiltered, scanline_width, stride, md);

                _set_color(unfiltered, stride, md, y, is_gray, is_plt, is_alpha);
            }

            free(unfiltered);
            break;
        }

        case PNG_CS_PLTE: {
            is_plt = 1;

            for (size_t y = 0; y < md->height; y++) {
                unfilter_png(md->image_data[y * scanline_width], // Filter type
                            y, unfiltered, scanline_width, stride, md);

                _set_color(unfiltered, stride, md, y, is_gray, is_plt, is_alpha);
            }

            free(unfiltered);
            break;
        }

        case PNG_CS_RGB: {
            for (size_t y = 0; y < md->height; y++) {
                unfilter_png(md->image_data[y * scanline_width], y, unfiltered,
                            scanline_width, stride, md);

                _set_color(unfiltered, stride, md, y, is_gray, is_plt, is_alpha);
            }

            free(unfiltered);
            break;
        }

        case PNG_CS_RGB_ALPHA: {
            is_alpha = 1;

            for (size_t y = 0; y < md->height; y++) {
                unfilter_png(md->image_data[y * scanline_width], y, unfiltered,
                            scanline_width, stride, md);

                _set_color(unfiltered, stride, md, y, is_gray, is_plt, is_alpha);
            }

            free(unfiltered);
            break;
        }
    }

    return 0;
}

int uncompress_png(unsigned char *in_buf, unsigned char *out_buf,
                   const size_t in_buf_size, const size_t out_buf_size,
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

int __filter_sub(PNG_Metadata *md, uint16_t *unfiltered, const size_t row_idx) {
    /*
     * To remove the Sub filter, we will add the previous pixel
     * to the current pixel to reconstruct the current pixel.
     * It is always the case that the prev pixel of first pixel
     * in every scanline is 0.
     * */
    if (md->width < 1 || md->height < 1 || md->num_channels < 1) {
        fprintf(stderr, "Error: Could not apply filter; Invalid metadata!\n");
        return 1;
    }

    size_t scanline_width =
        md->width * md->num_channels * md->bytes_per_channel + 1;
    size_t stride = md->width * md->num_channels * md->bytes_per_channel;
    size_t decrement_sz = md->num_channels * md->bytes_per_channel;

    for (size_t x = 0; x < stride; x++) {
        size_t offset = row_idx * scanline_width + x + 1;
        uint32_t prev = (x >= decrement_sz)
                            ? unfiltered[row_idx * stride + (x - decrement_sz)]
                            : 0;

        unfiltered[row_idx * stride + x] =
            (md->image_data[offset] + prev) & 0xFF;
    }

    return 0;
}

int __filter_up(PNG_Metadata *md, uint16_t *unfiltered, const size_t row_idx) {
    /*
     * to remove the up filter, we will
     * add the previous pixel at the previous
     * row to the current pixel to reconstruct the
     * current pixel. it is always the case
     * that the prev pixel of the first pixel
     * in every scanline is 0.
     */
    if (md->width < 1 || md->height < 1 || md->num_channels < 1) {
        fprintf(stderr, "Error: Could not apply filter; Invalid metadata!\n");
        return 1;
    }

    size_t scanline_width =
        md->width * md->num_channels * md->bytes_per_channel + 1;
    size_t stride = md->width * md->num_channels * md->bytes_per_channel;

    for (size_t x = 0; x < stride; x++) {

        size_t offset = row_idx * scanline_width + x + 1;
        uint32_t prev = row_idx ? unfiltered[(row_idx - 1) * stride + x] : 0;

        unfiltered[row_idx * stride + x] =
            (md->image_data[offset] + prev) & 0xFF;
    }

    return 0;
}

int __filter_avg(PNG_Metadata *md, uint16_t *unfiltered, const size_t row_idx) {
    /*
     * This filter combines both the up and sub filters.
     * We desconstruct by combining the effects of both*/
    if (md->width < 1 || md->height < 1 || md->num_channels < 1) {
        fprintf(stderr, "Error: Could not apply filter; Invalid metadata!\n");
        return 1;
    }

    size_t scanline_width = md->width * md->num_channels * md->bytes_per_channel + 1;
    size_t stride = md->width * md->num_channels * md->bytes_per_channel;
    size_t decrement_sz = md->num_channels * md->bytes_per_channel;

    for (size_t x = 0; x < stride; x++) {

        size_t offset = row_idx * scanline_width + x + 1;
        uint32_t prev_a = (x >= decrement_sz)
                ? unfiltered[row_idx * stride + (x - decrement_sz)]
                : 0;
        uint32_t prev_b = row_idx ? unfiltered[(row_idx - 1) * stride + x] : 0;

        unfiltered[row_idx * stride + x] = md->image_data[offset] +
                (uint16_t)(floor((float)(prev_a + prev_b) / 2)) &
            0xFF;
    }

    return 0;
}

int __filter_paeth(PNG_Metadata *md, uint16_t *unfiltered,
                   const size_t row_idx) {

    if (md->width < 1 || md->height < 1 || md->num_channels < 1) {
        fprintf(stderr, "Error: Could not apply filter; Invalid metadata!\n");
        return 1;
    }

    size_t scanline_width =
        md->width * md->num_channels * md->bytes_per_channel + 1;
    size_t stride = md->width * md->num_channels * md->bytes_per_channel;
    size_t decrement_sz = md->num_channels * md->bytes_per_channel;

    for (size_t x = 0; x < stride; x++) {
        /*
         * using paeth predictor as shown at
         * https://www.w3.org/TR/png-3/#paethpredictor-function
         * */
        size_t offset = row_idx * scanline_width + x + 1;
        int32_t prev_a = (x >= decrement_sz)
                             ? unfiltered[row_idx * stride + (x - decrement_sz)]
                             : 0;
        int32_t prev_b = row_idx ? unfiltered[(row_idx - 1) * stride + x] : 0;
        int32_t prev_c =
            (x >= decrement_sz)
                ? unfiltered[(row_idx - 1) * stride + (x - decrement_sz)]
                : 0;

        int32_t _p = prev_a + prev_b - prev_c;
        int32_t _pa = abs(_p - prev_a);
        int32_t _pb = abs(_p - prev_b);
        int32_t _pc = abs(_p - prev_c);

        uint32_t _prev = 0;
        if (_pa <= _pb && _pa <= _pc) {
            _prev = prev_a;
        } else if (_pb <= _pc) {
            _prev = prev_b;
        } else {
            _prev = prev_c;
        }

        unfiltered[row_idx * stride + x] =
            (md->image_data[offset] + _prev) & 0xFF;
    }

    return 0;
}

int unfilter_png(const unsigned char ftype, const size_t row_idx,
                 uint16_t *unfiltered, const size_t scanline_width,
                 const size_t stride, PNG_Metadata *md) {

    switch (ftype) {
        case PNG_FILTER_NONE: {
            for (size_t x = 0; x < stride; x++) {
                unfiltered[row_idx * stride + x] = md->image_data[row_idx * scanline_width + x + 1];
            }
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
