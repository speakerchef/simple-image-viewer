#include "include/jpeg.h"
#include "include/utils.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


int decode_jpeg(FILE *file, size_t file_sz){
    const unsigned char APP0_mk[2] = {0xff, 0xe0};
    JPEG_Metadata jpeg_metadata = {0};

    fseek(file, 0, SEEK_SET);

    bool terminate = 0;
    static size_t dht_count = 0;

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
                // In progress: Store each huffman table in array
                // TODO: Figure out selection based on defined index
                // TODO: Parse
                //NOTE: B.2.4.2

                fseek(file, -2, SEEK_CUR);
                printf("DHT\n");
                printf("Chunk size: %u\n", chunk_sz);
                if (!(jpeg_metadata.huffman_table = calloc(sizeof(char), chunk_sz))) { return PROC_FAILURE; };
                if (!fread(jpeg_metadata.huffman_table, sizeof(char), chunk_sz, file)) { goto cleanup; };





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
                if (!(jpeg_metadata.qtable_data = calloc(sizeof(char), chunk_sz))) { return PROC_FAILURE; };
                if (!fread(jpeg_metadata.qtable_data, sizeof(char), chunk_sz, file)) { goto cleanup; };

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
                if (!(jpeg_metadata.restart_interval = calloc(sizeof(char), chunk_sz))) { return PROC_FAILURE; };
                if (!fread(jpeg_metadata.restart_interval, sizeof(char), chunk_sz, file)) { goto cleanup; }

                for (size_t i = 0; i < chunk_sz; i++) {
                    printf("%02x ", jpeg_metadata.restart_interval[i]);
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
                if (!scan_header_data) { return PROC_FAILURE; }
                unsigned char *temp = calloc(1, sizeof(char));
                if (!temp) { return PROC_FAILURE; }

                if (!fread(scan_header_data, sizeof(char), chunk_sz, file)) { goto cleanup; };
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

                free(scan_header_data);

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

cleanup:
    free(jpeg_metadata.image_data);
    free(jpeg_metadata.huffman_coding_data);
    free(jpeg_metadata.dht_table);
    free(jpeg_metadata.huffman_table);
    free(jpeg_metadata.qtable_data);
    free(jpeg_metadata.restart_interval);
 

    return 0;
}
