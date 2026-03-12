#include "include/jpeg.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


int decode_jpeg(FILE *file, size_t file_sz){
    const unsigned char APP0_mk[2] = {0xff, 0xe0};
    JPEG_Metadata jpeg_metadata = {0};


    unsigned char APP0_ext_Marker[2]; // Used to check if extension segment exists
    bool has_extension = false;

    unsigned char APP0_ext_len[2];
    unsigned char APP0_ext_ident[5];
    unsigned char APP0_ext_thumb_fmt;
    unsigned char *thumbnail_dat = NULL;
    
    fread(jpeg_metadata.APP0_Marker, sizeof(char), 2, file);
    fread(&jpeg_metadata.APP0_len, sizeof(char), 2, file);
    fread(jpeg_metadata.APP0_ident, sizeof(char), 5, file);
    fread(jpeg_metadata.JFIF_Vers, sizeof(char), 2, file);
    fread(&jpeg_metadata.density_unit, sizeof(char), 1, file);
    fread(&jpeg_metadata.x_dens, sizeof(char), 2, file);
    fread(&jpeg_metadata.y_dens, sizeof(char), 2, file);
    fread(&jpeg_metadata.thumbnail_w, sizeof(char), 1, file);
    fread(&jpeg_metadata.thumbnail_h, sizeof(char), 1, file);
    fread(APP0_ext_Marker, sizeof(char), 2, file);


    if (!memcmp(APP0_ext_Marker, APP0_mk, 2)) {
        printf("There is an extension\n");
        fread(APP0_ext_len, sizeof(char), 2, file);
        fread(APP0_ext_ident, sizeof(char), 5, file);
        fread(&APP0_ext_thumb_fmt, sizeof(char), 1, file);
    } else {
        fseek(file, -2, SEEK_CUR);
    }

    /* Print binary for debug */
    // fseek(file, 0, SEEK_SET);
    // unsigned char total_dat[file_sz];
    // fread(total_dat, sizeof(char), file_sz, file);
    // for (size_t i = 0; i < file_sz; i++) {
    //     printf("%02x ", total_dat[i]);
    // }
    // printf("\n");
    /**/

    fseek(file, 0, SEEK_SET);


    printf("APPO len: %u, JFIF_Vers: %02x %02x, \
            \nthumb_w: %02x, thumb_h: %02x, Density Unit: %02x, \
            \nyDens: %u, xDens: %u\n", 
           jpeg_metadata.APP0_len, 
           jpeg_metadata.JFIF_Vers[0], 
           jpeg_metadata.JFIF_Vers[1], 
           jpeg_metadata.thumbnail_w, 
           jpeg_metadata.thumbnail_h, 
           jpeg_metadata.density_unit, 
           jpeg_metadata.y_dens, 
           jpeg_metadata.x_dens);

    printf("\n");

    bool terminate = 0;

    while (!terminate) {

        uint16_t file_marker;
        uint16_t chunk_sz;

        if (!fread(&file_marker, sizeof(char), 2, file)) { break; }
        if (!fread(&chunk_sz, sizeof(char), 2, file)) { break; }

        file_marker = ntohs(file_marker);
        chunk_sz = ntohs(chunk_sz);

        // printf("file marker: %u\n", file_marker);
        // printf("Chunk size: %u\n", chunk_sz);

        if (file_marker == HUFFMAN_BASELINE_DCT         ||
            file_marker == HUFFMAN_EXTENDED_SEQ_DCT     || 
            file_marker == HUFFMAN_PROGRESSIVE_DCT      ||
            file_marker == HUFFMAN_LOSSLESS             ||
            file_marker == HUFFMAN_DIFF_SEQ_DCT         ||
            file_marker == HUFFMAN_DIFF_PROGRESSIVE_DCT ||
            file_marker == HUFFMAN_DIFF_LOSSLESS_DCT) {

            fseek(file, -2, SEEK_CUR);
            jpeg_metadata.huffman_coding_type = file_marker;

            printf("Huffman Marker: %u\n", file_marker);
            printf("Chunk Size: %u \n", chunk_sz);

            /* Implementation */
            jpeg_metadata.huffman_coding_data = calloc(chunk_sz, sizeof(char));
            fread(jpeg_metadata.huffman_coding_data, sizeof(char), chunk_sz, file);
            
            // printf("Huffman coding data: ");
            // for (size_t i = 0; i < chunk_sz; i++) {
            //     printf("%02x ", jpeg_metadata.huffman_coding_data[i]);
            // }
            // printf("\n");
       }

        switch (file_marker) {

            case HUFFMAN_DHT: {
                // TODO: Store each huffman table in array
                // TODO: Figure out selection based on defined index
                // TODO: Parse
                //NOTE: B.2.4.2

                fseek(file, -2, SEEK_CUR);
                printf("DHT\n");
                printf("Chunk size: %u\n", chunk_sz);
                jpeg_metadata.huffman_table = calloc(sizeof(char), chunk_sz);
                fread(jpeg_metadata.huffman_table, sizeof(char), chunk_sz, file);

                for (size_t i = 0; i < chunk_sz; i++) {
                    printf("%02x ", jpeg_metadata.huffman_table[i]);
                }
                printf("\n");

                // fseek(file, -(short)chunk_sz, SEEK_CUR);
                break;
            }


            case JPEG_DQT: {
                //TODO: Parse
                //NOTE: B.2.4.1
                fseek(file, -2, SEEK_CUR);
                printf("DQT\n");
                printf("Chunk size: %u\n", chunk_sz);
                jpeg_metadata.qtable_data = calloc(sizeof(char), chunk_sz);
                fread(jpeg_metadata.qtable_data, sizeof(char), chunk_sz, file);

                for (size_t i = 0; i < chunk_sz; i++) {
                    printf("%02x ", jpeg_metadata.qtable_data[i]);
                }
                printf("\n");

                break; }


            case JPEG_DRI: {
                // TODO: Parse
                //NOTE: B.2.4.4
                fseek(file, -2, SEEK_CUR);
                printf("DRI\n");
                printf("Chunk size: %u\n", chunk_sz);
                jpeg_metadata.restart_interval = calloc(chunk_sz, sizeof(char));

                for (size_t i = 0; i < chunk_sz; i++) {
                    printf("%02x ", jpeg_metadata.qtable_data[i]);
                }
                printf("\n");
                break;
            }


            case JPEG_SOS: {
                // TODO: Dispatch info to right location
                fseek(file, -2, SEEK_CUR);
                printf("SOS\n");
                printf("Chunk size: %u\n", chunk_sz);

                unsigned char *scan_header_data = calloc(chunk_sz, sizeof(char));
                unsigned char *temp = calloc(1, sizeof(char));

                fread(scan_header_data, sizeof(char), chunk_sz, file);
                for (size_t i = 0; i < chunk_sz; i++) {
                    printf("%02x ", scan_header_data[i]);
                }
                printf("\n");


                // Read in scan header params
                // NOTE: No checking against huffman coding type
                // -- Assuming baseline or extended DCT
                // Table B.3 (itu-T81)
                uint8_t num_components; // Param used for j (Cs_j, Td_j, Ta_j)
                uint8_t seq_len;

                num_components = scan_header_data[2];
                printf("Num components: %u\n", num_components);
                seq_len = 6 + 2 * num_components;

                uint8_t Cs_scan_com_selec[num_components];
                uint8_t Td_dht_choice_selec[num_components]; // Out of 4 available DHT
                uint8_t Ta_ac_selec[num_components]; // Not used (arithmetic coding)

                uint8_t Ss_first_dct_coeff_selectn; // (start of spectral/predictor selection)
                uint8_t Se_end_spect_selectn;
                uint8_t Ah_approx_highbit;
                uint8_t Ah_approx_lowbit;

                

                //Reading in header components
                for (long j = 0; j < num_components; j++) {
                    size_t stride = j * 2;
                    Cs_scan_com_selec[j] = scan_header_data[3 + stride];
                    Td_dht_choice_selec[j] = ( scan_header_data[3 + stride + 1] >> 4 ) & 0x0F;
                    Ta_ac_selec[j] = scan_header_data[3 + stride + 1] & 0x0F;
                }

                Ss_first_dct_coeff_selectn = scan_header_data[seq_len - 3];
                Se_end_spect_selectn = scan_header_data[seq_len - 2];
                Ah_approx_highbit = ( scan_header_data[seq_len - 1] >> 4 ) & 0x0F;
                Ah_approx_lowbit = ( scan_header_data[seq_len - 1] ) & 0x0F;

                // test variable chunks
                for (int i = 0; i < num_components; i++) {
                    printf("Cs_j = %u ", Cs_scan_com_selec[i]);
                    printf("Td_j = %u ", Td_dht_choice_selec[i]);
                    printf("Ta_j = %u\n", Ta_ac_selec[i]);
                }
                printf("Ss: %u, Se: %u, Ah_high: %u, Ah_low: %u \n", 
                       Ss_first_dct_coeff_selectn,
                       Se_end_spect_selectn,
                       Ah_approx_highbit,
                       Ah_approx_lowbit);



                // Read in image data
                size_t counter = 0;
                while (fread(&temp[counter], sizeof(char), 1, file) &&
                        ntohs( temp[counter] ) != JPEG_EOI){

                    counter++;
                    temp = realloc(temp, counter + 1);
                };


                size_t offset = counter - 2;
                jpeg_metadata.image_data = calloc(offset, sizeof(char));
                for (size_t i = 0; i < offset; i++) {
                    jpeg_metadata.image_data[i] = temp[i];
                }
                free(temp);
                

                printf("EOI\n");
                printf("Image data: \n");
                // for (size_t i = 0; i < offset; i++) {
                //     printf("%02x ", jpeg_metadata.image_data[i]);
                // }
                printf("\n");
                terminate = 1;
                break;
            }

        }
    }
    printf("\n");

    return 0;
}
