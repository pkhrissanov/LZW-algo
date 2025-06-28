#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_DICT_SIZE 8192
#define WORD_LEN 8192
#define CLEAR_CODE 256
#define END_CODE 257
#define INITIAL_CODE_SIZE 10
#define MAX_BITS 13

uint64_t bit_buffer = 0;
int bit_count = 0;

// Reads a code of 'sz' bits from the input stream
int read_bits(FILE* in, int sz) {
    while (bit_count < sz) {
        int byte = fgetc(in);
        if (byte == EOF) return -1;
        bit_buffer = (bit_buffer << 8) | byte;
        bit_count += 8;
    }

    int shift = bit_count - sz;
    int code = (bit_buffer >> shift) & ((1 << sz) - 1);
    bit_count -= sz;
    bit_buffer &= (1ULL << bit_count) - 1;
    return code;
}

// Initializes dictionary with 0-255 characters
void dict_init(char** dict, int* nextCode) {
    for (int i = 0; i < 256; i++) {
        dict[i] = malloc(2);
        dict[i][0] = (char)i;
        dict[i][1] = '\0';
    }
    for (int i = 256; i < MAX_DICT_SIZE; i++) {
        free(dict[i]);
        dict[i] = NULL;
    }
    *nextCode = 258;
}

void decompress(FILE* in, FILE* out) {
    char* dict[MAX_DICT_SIZE] = {0};
    int nextCode;
    int codeSize = INITIAL_CODE_SIZE;

    dict_init(dict, &nextCode);

    int prev_code = read_bits(in, codeSize);
    if (prev_code < 0 || prev_code >= MAX_DICT_SIZE || !dict[prev_code]) {
        fprintf(stderr, "Invalid first code\n");
        return;
    }

    fputs(dict[prev_code], out);

    while (1) {
        int code = read_bits(in, codeSize);
        if (code == -1) break;
        if (code == END_CODE) break;

        if (code == CLEAR_CODE) {
            dict_init(dict, &nextCode);
            codeSize = INITIAL_CODE_SIZE;

            // Read a fresh new starting code
            prev_code = read_bits(in, codeSize);
            if (prev_code < 0 || !dict[prev_code]) {
                fprintf(stderr, "Invalid code after CLEAR_CODE\n");
                return;
            }
            fputs(dict[prev_code], out);
            continue;
        }

        char* entry = NULL;
        bool special_case = false;
        
        if (code < MAX_DICT_SIZE && dict[code]) {
            entry = dict[code];
        } else if (code == nextCode) {
            size_t len = strlen(dict[prev_code]);
            entry = malloc(len + 2);
            strcpy(entry, dict[prev_code]);
            entry[len] = dict[prev_code][0];
            entry[len + 1] = '\0';
            dict[nextCode] = entry;
            special_case = true;
            
            if ((nextCode + 1) == (1 << codeSize) && codeSize < MAX_BITS) {
                codeSize++;
            }
            
            nextCode++;
        } else {
            fprintf(stderr, "Invalid code: %d (nextCode: %d, MAX_DICT_SIZE: %d)\n", code, nextCode, MAX_DICT_SIZE);
            return;
        }

        fputs(entry, out);

        // Add new entry to dictionary (only if not special case)
        if (!special_case && nextCode < MAX_DICT_SIZE) {
            size_t len = strlen(dict[prev_code]) + 2;
            char* new_entry = malloc(len);
            snprintf(new_entry, len, "%s%c", dict[prev_code], entry[0]);
            dict[nextCode] = new_entry;
            
            if ((nextCode + 1) == (1 << codeSize) && codeSize < MAX_BITS) {
                codeSize++;
            }
            
            nextCode++;
        }

        prev_code = code;
    }

    for (int i = 0; i < MAX_DICT_SIZE; i++) {
        free(dict[i]);
    }
}

int main() {
    char in_fn[] = "incomp";
    char out_fn[] = "outdecomp";

    FILE* in = fopen(in_fn, "rb");
    if (!in) { perror("Open input failed"); return 1; }

    FILE* out = fopen(out_fn, "wb");
    if (!out) { perror("Open output failed"); fclose(in); return 1; }

    decompress(in, out);

    fclose(in);
    fclose(out);
    return 0;
}
