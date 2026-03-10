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

// TODO: Support 1, 2, 4 bit depth color spaces
// TODO: Gamma gAMA chunk handling
// TODO: iCCP Chunk
// TODO: sRGB chunk

RenderData *decode_png(FILE *file) {

    PNG_Metadata png_metadata = {0};

    RenderData *renderData = NULL;

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

            if (ret != 13) { goto cleanup; }

            png_metadata.width = ntohl(png_metadata.width);
            png_metadata.height = ntohl(png_metadata.height);
            png_metadata.bytes_per_channel = png_metadata.bit_depth / 8;

            // Set color space information
            switch (png_metadata.color_space) {
            case PNG_CS_GRAY: {
                png_metadata.num_channels = 1;
                png_metadata.is_gray = 1;
                break;
            }
            case PNG_CS_RGB: {
                png_metadata.num_channels = 3;
                break;
            }
            case PNG_CS_PLTE: {
                png_metadata.num_channels = 1;
                png_metadata.is_plt = 1;
                break;
            }
            case PNG_CS_GRAY_ALPHA: {
                png_metadata.num_channels = 2;
                png_metadata.is_gray = 1;
                png_metadata.is_alpha = 1;
                break;
            }
            case PNG_CS_RGB_ALPHA: {
                png_metadata.num_channels = 4;
                png_metadata.is_alpha = 1;
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
            if (chunk_sz != 4) { goto cleanup; }
            unsigned char cicp_dat[chunk_sz]; 
            fread(cicp_dat, sizeof(char), chunk_sz, file);

            if (cicp_dat[0] != 0x09) { fprintf(stderr, WARN_BAD_DATA); }

            for (size_t i = 0; i < chunk_sz; i++) {
                printf("%02x ", cicp_dat[i]); 
            }
            printf("\n");

            switch (cicp_dat[1]){
                case 0x10: {
                    png_metadata.is_pq = 1;
                    break;
                }
                case 0x12: {
                    png_metadata.is_hlg = 1;
                    break;
                }
                default: { break; }
            }

            png_metadata.is_hdr = 1;

            fseek(file, CRC_SZ, SEEK_CUR);
            continue;
        }

        if (!memcmp(chunk_type, "iCCP", 4)) {
        }

        if (!memcmp(chunk_type, "gAMA", 4)) {
            if (png_metadata.is_hdr) {
                fseek(file, CRC_SZ, SEEK_CUR); 
                continue;
            }
            if (chunk_sz != 4) { goto cleanup; }
 
            unsigned char gamma[4] = {0};
            fread(gamma, chunk_sz, 1, file);

            printf("gAMA chunk data: %s \n", gamma);

            fseek(file, CRC_SZ, SEEK_CUR);
            continue;

        }

        // Extract color palette information if available
        if (!memcmp(chunk_type, "PLTE", 4)) {
            if (chunk_sz % 3 != 0) { goto cleanup; }

            png_metadata.palette = calloc(chunk_sz, sizeof(char));
            if (!png_metadata.palette) { goto cleanup; }
            fread(png_metadata.palette, sizeof(char), chunk_sz, file);

            fseek(file, CRC_SZ, SEEK_CUR);
            continue;
        }

        if (!memcmp(chunk_type, "tRNS", 4)) {
            uint8_t span = PNG_CS_PLTE ? 1 : 2; // PLTE tRNS chunk has 1 bytes sequences 
            png_metadata.transparency = calloc(1, chunk_sz);
            if (!png_metadata.transparency) { goto cleanup; }

            fread(png_metadata.transparency, sizeof(char), chunk_sz, file);
            png_metadata.trns_sz = chunk_sz;

            fseek(file, CRC_SZ, SEEK_CUR);
            continue;
        }

        // Get and set background colors if defined
        if (!memcmp(chunk_type, "bKGD", 4)) {
            if (png_metadata.color_space == PNG_CS_GRAY || png_metadata.color_space == PNG_CS_GRAY_ALPHA) {
                if (chunk_sz != 2) { goto cleanup; }

                unsigned char bkgd_dat[chunk_sz];
                fread(bkgd_dat, sizeof(char), chunk_sz, file);
                png_metadata.set_bg = 1;

                png_metadata.bg_color.r = (( bkgd_dat[0] << 8 ) | bkgd_dat[1]) >> 8;
                png_metadata.bg_color.a = UINT8_MAX;

            } else if (png_metadata.color_space == PNG_CS_RGB || png_metadata.color_space == PNG_CS_RGB_ALPHA) {
                if (chunk_sz != 6) { goto cleanup; }

                unsigned char bkgd_dat[chunk_sz];
                fread(bkgd_dat, sizeof(char), chunk_sz, file);
                png_metadata.set_bg = 1;

                png_metadata.bg_color.r = (( bkgd_dat[0] << 8 ) | bkgd_dat[1]) >> 8;
                png_metadata.bg_color.g = (( bkgd_dat[2] << 8 ) | bkgd_dat[3]) >> 8;
                png_metadata.bg_color.b = (( bkgd_dat[4] << 8 ) | bkgd_dat[5]) >> 8;
                png_metadata.bg_color.a = UINT8_MAX;

            } else if (png_metadata.color_space == PNG_CS_PLTE) {
                if (chunk_sz != 1) { goto cleanup; }
                unsigned char bkgd_dat;
                fread(&bkgd_dat, sizeof(char), chunk_sz, file);
                png_metadata.bg_color.r = png_metadata.palette[bkgd_dat];
                png_metadata.bg_color.g = png_metadata.palette[bkgd_dat + 1];
                png_metadata.bg_color.b = png_metadata.palette[bkgd_dat + 2];
                png_metadata.bg_color.a = UINT8_MAX;
                // printf("r: %u, g: %u, b: %u \n", png_metadata.bg_color.r, png_metadata.bg_color.g, png_metadata.bg_color.b);
            } else { goto cleanup; }


            fseek(file, CRC_SZ, SEEK_CUR);
            continue;
        }

        // Extract image data and append sequential IDAT chunks 
        if (!memcmp(chunk_type, "IDAT", 4)) {
            png_metadata.image_data = realloc( png_metadata.image_data,
                                              png_metadata.total_size + chunk_sz);
            if (!png_metadata.image_data) { goto cleanup; }

            fread(png_metadata.image_data + png_metadata.total_size, 
                  1,
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

    size_t scanline_size = (png_metadata.width * 
                            png_metadata.num_channels * 

                            png_metadata.bit_depth + 7) / 8;

    size_t output_size = png_metadata.height *
                         (scanline_size + 1); // +1 to account for filter byte

    unsigned char *output_buffer = calloc(output_size, sizeof(unsigned char));
    if (!output_buffer) { goto cleanup; }

    int ret = uncompress_png(png_metadata.image_data, output_buffer,
                       png_metadata.total_size, output_size, &png_metadata);

    png_metadata.alpha_data = 0;
    ret = ret & load_png_colors(&png_metadata, png_metadata.alpha_data);
    if (ret) { goto cleanup; }

    renderData = calloc(1, sizeof(RenderData));
    if (!renderData) { goto cleanup; }
    renderData->color = png_metadata.pixel_color;
    renderData->width = png_metadata.width;
    renderData->height = png_metadata.height;
    renderData->bytes_per_channel = png_metadata.bytes_per_channel;
    renderData->num_channels = png_metadata.num_channels;
    renderData->bg_color = png_metadata.bg_color;
    renderData->set_bg = png_metadata.set_bg;
    renderData->ret = ret;

    cleanup:
        free(png_metadata.palette);
        free(png_metadata.transparency);
        free(png_metadata.image_data);
        return renderData;

}

void _set_color(uint16_t *unfiltered, 
                const size_t stride, 
                PNG_Metadata *md,
                const size_t y
                ) {

    uint8_t desired_bdepth = 8;
    bool is_quant = 0;
    uint16_t r, g, b, a;

    for (size_t x = 0; x < md->width; x++) {

        size_t offset = (y * stride) + (x * md->num_channels * md->bytes_per_channel);
        uint16_t index = unfiltered[offset];
        uint16_t palette_stride = index * 3;


        if (md->bytes_per_channel > 1) {
            r = (unfiltered[offset] << desired_bdepth) | unfiltered[offset + 1];
            g = (unfiltered[offset + 2] << desired_bdepth) | unfiltered[offset + 3];
            b = (unfiltered[offset + 4] << desired_bdepth) | unfiltered[offset + 5];
            a = md->is_alpha ? (unfiltered[offset + 6] << desired_bdepth) | unfiltered[offset + 7]
                         : UINT16_MAX;


            if (md->is_hdr) {
                double lin_rgb[3] = {0};

                lin_rgb[0] = (md->is_pq ? pq_transfer_func(&r) / BT2100_REF_WHITE : hlg_transfer_func(&r));
                lin_rgb[1] = (md->is_pq ? pq_transfer_func(&g) / BT2100_REF_WHITE : hlg_transfer_func(&g)); 
                lin_rgb[2] = (md->is_pq ? pq_transfer_func(&b) / BT2100_REF_WHITE : hlg_transfer_func(&b));

                if (md->is_hlg) {
                    const double Y_s = 
                        0.2627 * lin_rgb[0] + 
                        0.678 * lin_rgb[1] + 
                        0.0593 * lin_rgb[2];

                    _hlg_OOTF(&lin_rgb[0], &Y_s);
                    _hlg_OOTF(&lin_rgb[1], &Y_s);
                    _hlg_OOTF(&lin_rgb[2], &Y_s);

                    double norm_const = 10000. * BT2100_REF_WHITE;
                    lin_rgb[0] /= norm_const;
                    lin_rgb[1] /= norm_const;
                    lin_rgb[2] /= norm_const;
                } 


                Matrix XYZ = {lin_rgb, 1, 3};
                Matrix result = {0};
                result.rows = XYZ.rows;
                result.cols = xyz2rgb_mat.cols;
                result.coeffs = malloc(sizeof(double) * (result.rows * result.cols));
                if (!result.coeffs) { exit(1); }

                _matrix_mult(&XYZ, &xyz2rgb_mat, &result);

                                                                    
                CLAMP(result.coeffs[0], 1., 0.);                    
                CLAMP(result.coeffs[1], 1., 0.);
                CLAMP(result.coeffs[2], 1., 0.);

                r = round(result.coeffs[0] * UINT16_MAX);
                g = round(result.coeffs[1] * UINT16_MAX);
                b = round(result.coeffs[2] * UINT16_MAX);
                // r = round(lin_rgb[0] * UINT16_MAX);
                // g = round(lin_rgb[1] * UINT16_MAX);
                // b = round(lin_rgb[2] * UINT16_MAX);

                free(result.coeffs);
            }
        } else {
            if (md->is_plt) {
                r = md->palette[palette_stride];
                g = md->palette[palette_stride + 1];
                b = md->palette[palette_stride + 2];
                a = ( index < md->trns_sz ) ? md->transparency[index]
                                            : UINT16_MAX;
            } else {
                r = unfiltered[offset];
                g = unfiltered[offset + 1];
                b = unfiltered[offset + 2];
                a = md->is_alpha ? unfiltered[offset + 3]
                             : UINT16_MAX;
            }

            r <<= 8;
            g <<= 8;
            b <<= 8;
        }

        if (md->is_gray) {
            md->pixel_color[y * md->width + x] = (ColorData16){r, r, r, a};
        } else {
            md->pixel_color[y * md->width + x] = (ColorData16){r, g, b, a};
        }

    }
}

void apply_R709_gamma(double *I) {
    *I = MIN(1., *I);
    *I = MAX(0., *I);
    if (*I < 0.018) {
        *I = 4.5 * *I;
    } else {
        *I = 1.099 * pow(*I, 0.45) - 0.099;
    }
}
void apply_sRGB_gamma(double *I) {
    *I = MIN(1., *I);
    *I = MAX(0., *I);
    if (*I <= 0.0031308) {
        *I = 12.92 * *I;
    } else {
        *I = 1.055 * pow(*I, 1. / REC_709_GAMMA) - 0.055;
    }
}

double pq_transfer_func(uint16_t *E_pr) {
    /*
     * Non-linear to linear space
     * As defined in Rec. ITU-R BT.2100-3
     * */
    const double input = (double)*E_pr / UINT16_MAX;
    const double m2_const = (1 / _M2);
    const double m1_const = (1 / _M1);

    double _max = MAX( (pow(input, m2_const) - _C1), 0.);
    double _dc3 =  _C3 * pow(input, m2_const);
    double _b = fabs(_max / ( _C2 - _dc3 ));
    double Y = pow(_b, m1_const); // Linear normalized color in XYZ 

    return Y;
}

void _hlg_OOTF(double *E, const double *Y_s) {
    double alpha = 1000.; 
    *E = alpha * pow(*Y_s, HLG_REF_GAMMA - 1) * *E;
}

double hlg_transfer_func(uint16_t *E_pr) {
    double res = 0;
    double input = (double)*E_pr / UINT16_MAX;

    if (input <= 0.5) {
        res = ( input * input ) / 3.;
    } else {
        res = ( (exp((input - HLG_C) / HLG_A) + HLG_B) ) / 12.;
    }
    
    return res;
}


int load_png_colors(PNG_Metadata *md, uint16_t alpha_data) {

    if (md->width < 1 || md->height < 1 || md->num_channels < 1) {
        fprintf(stderr, ERR_BAD_FILE);
        return 1;
    }

    size_t alloc_sz = 0;
    if (md->bit_depth <= 8) { alloc_sz = sizeof(ColorData8); }
    else { alloc_sz = sizeof(ColorData16); }

    md->pixel_color = calloc(md->width * md->height, alloc_sz);
    if (!md->pixel_color) { return 1; }

    size_t stride = md->width * md->num_channels * md->bytes_per_channel;
    size_t scanline_width = stride + 1;
    uint16_t *unfiltered = calloc(md->height * stride, sizeof(uint16_t));
    if (!unfiltered) { return 1; }

    for (size_t y = 0; y < md->height; y++) {
        unfilter_png(md->image_data[y * scanline_width], // Filter type
                    y, unfiltered, scanline_width, stride, md);

        _set_color(unfiltered, stride, md, y);
    }

    free(unfiltered);
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
