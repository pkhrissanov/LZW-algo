#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define TABLE_SIZE 4096
#define WORD_LEN 100
#define DEBUG true
#define INITIAL_TABLE_SIZE 4096
#define LOAD_FACTOR_THRESHOLD 0.75
#define MAX_DICT_SIZE 4096
#define RESET_MARKER 4095
#define END_MARKER 4094

uint32_t bit_buffer = 0;
int bit_count = 0;

void flush_bits(FILE *out) {
    while (bit_count > 0) {
        if (bit_count >= 8) {
            uint8_t byte = (bit_buffer >> (bit_count - 8)) & 0xFF;
            fputc(byte, out);
            bit_count -= 8;
        } else {
            uint8_t byte = (bit_buffer << (8 - bit_count)) & 0xFF;
            fputc(byte, out);
            bit_count = 0;
        }
    }
    bit_buffer = 0;
}

int read_bits(FILE *in, int code_size) {
    while (bit_count < code_size) {
        int byte = fgetc(in);
        if (byte == EOF) return -1;
        bit_buffer = (bit_buffer << 8) | byte;
        bit_count += 8;
    }
    int shift = bit_count - code_size;
    int code = (bit_buffer >> shift) & ((1 << code_size) - 1);
    bit_count -= code_size;
    bit_buffer &= (1 << bit_count) - 1;
    return code;
}

char* dictionary[MAX_DICT_SIZE];

void dict_reset(int* next_code, int* code_size) {
    for (int i = 0; i < MAX_DICT_SIZE; i++) {
        if (dictionary[i]) free(dictionary[i]);
        dictionary[i] = NULL;
    }
    for (int i = 0; i < 256; i++) {
        dictionary[i] = malloc(2);
        dictionary[i][0] = (char)i;
        dictionary[i][1] = '\0';
    }
    *next_code = 256;
    *code_size = 9;
    bit_buffer = 0;
    bit_count = 0;
}

int main() {
    char input_file[256], output_file[256];
    printf("Enter compressed file name: ");
    scanf("%255s", input_file);
    printf("Enter output file name: ");
    scanf("%255s", output_file);

    FILE* in = fopen(input_file, "rb");
    FILE* out = fopen(output_file, "wb");
    if (!in || !out) {
        perror("File open failed");
        return 1;
    }

    int code_size = 9, next_code;
    dict_reset(&next_code, &code_size);

    int prev_code = read_bits(in, code_size);
    if (prev_code < 0 || prev_code >= MAX_DICT_SIZE || !dictionary[prev_code]) {
        fprintf(stderr, "Invalid first code: %d\n", prev_code);
        return 1;
    }
    if (DEBUG) fprintf(stderr, "[INIT] First code: %d = %s\n", prev_code, dictionary[prev_code]);
    fwrite(dictionary[prev_code], 1, strlen(dictionary[prev_code]), out);

    while (1) {
        // 🟢 FIX: Increase code_size early using >=
        if (next_code >= 2048) code_size = 12;
        else if (next_code >= 1024) code_size = 11;
        else if (next_code >= 512) code_size = 10;

        int curr_code = read_bits(in, code_size);
        if (curr_code == -1) break;

        if (curr_code == END_MARKER) {
            if (DEBUG) fprintf(stderr, "[INFO] Reached END_MARKER.\n");
            break;
        }

        if (curr_code == RESET_MARKER) {
            if (DEBUG) fprintf(stderr, "[INFO] Reset marker encountered.\n");
            dict_reset(&next_code, &code_size);
            prev_code = read_bits(in, code_size);
            if (prev_code < 0 || !dictionary[prev_code]) {
                fprintf(stderr, "Invalid code after reset (%d)\n", prev_code);
                break;
            }
            fwrite(dictionary[prev_code], 1, strlen(dictionary[prev_code]), out);
            continue;
        }

        if (curr_code >= MAX_DICT_SIZE) {
            fprintf(stderr, "Invalid code (out of bounds): %d\n", curr_code);
            break;
        }

        if (DEBUG) fprintf(stderr, "[READ] Code: %d | next_code: %d | code_size: %d\n", curr_code, next_code, code_size);

        char* entry = NULL;

        if (curr_code == next_code) {
            if (!dictionary[prev_code]) {
                fprintf(stderr, "Invalid prev_code %d when curr_code == next_code\n", prev_code);
                break;
            }

            int len = strlen(dictionary[prev_code]);
            entry = malloc(len + 2);
            strcpy(entry, dictionary[prev_code]);
            entry[len] = dictionary[prev_code][0];
            entry[len + 1] = '\0';
        } else if (dictionary[curr_code]) {
            entry = dictionary[curr_code];
        } else {
            fprintf(stderr, "Invalid or unknown code: %d\n", curr_code);
            break;
        }

        fwrite(entry, 1, strlen(entry), out);

        if (next_code < MAX_DICT_SIZE && dictionary[prev_code]) {
            int len = strlen(dictionary[prev_code]);
            char* new_entry = malloc(len + 2);
            strcpy(new_entry, dictionary[prev_code]);
            new_entry[len] = entry[0];
            new_entry[len + 1] = '\0';
            dictionary[next_code++] = new_entry;
        }

        if (curr_code == next_code - 1 && entry != dictionary[curr_code]) {
            free(entry);
        }

        prev_code = curr_code;
    }

    for (int i = 0; i < MAX_DICT_SIZE; i++) free(dictionary[i]);
    fclose(in);
    fclose(out);
    return 0;
}
