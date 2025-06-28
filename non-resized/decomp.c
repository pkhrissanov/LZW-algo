#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_DICT_SIZE 4096
#define WORD_LEN 8192
#define BIT_BUMP_MARKER 4095
#define INITIAL_CODE_SIZE 8
#define MAX_BITS 12
#define DEBUG 1  // Enable debug output

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
    *nextCode = 256;
}

void decompress(FILE* in, FILE* out) {
    char* dict[MAX_DICT_SIZE] = {0};
    int nextCode;
    int codeSize = INITIAL_CODE_SIZE;
    int prev_code;

    dict_init(dict, &nextCode);

    prev_code = read_bits(in, codeSize);
    if (prev_code < 0 || prev_code >= MAX_DICT_SIZE || !dict[prev_code]) {
        fprintf(stderr, "Invalid first code: %d\n", prev_code);
        return;
    }

    if (DEBUG) printf("Initial code: %d → '%s'\n", prev_code, dict[prev_code]);
    fputs(dict[prev_code], out);

    while (1) {
        int code = read_bits(in, codeSize);
        if (code == -1) break;

        if (code == BIT_BUMP_MARKER) {
            if (DEBUG) printf("Bit size bump marker (code %d) encountered\n", BIT_BUMP_MARKER);
            if (codeSize < MAX_BITS) {
                codeSize++;
                if (DEBUG) printf("Bit size increased to %d\n", codeSize);
                continue;
            } else {
                fprintf(stderr, "Unexpected bit bump marker at max code size\n");
                break;
            }
        }

        if (DEBUG) printf("Read code: %d\n", code);

        char* entry = NULL;
        if (dict[code]) {
            entry = dict[code];
            if (DEBUG) printf("Code %d → '%s'\n", code, entry);
        } else if (code == nextCode) {
            size_t len = strlen(dict[prev_code]);
            entry = malloc(len + 2);
            strcpy(entry, dict[prev_code]);
            entry[len] = dict[prev_code][0];
            entry[len + 1] = '\0';
            dict[code] = entry;
            if (DEBUG) printf("Special case: Code %d built as '%s'\n", code, entry);
        } else {
            fprintf(stderr, "Invalid code: %d\n", code);
            break;
        }

        fputs(entry, out);

        if (nextCode < MAX_DICT_SIZE) {
            size_t len = strlen(dict[prev_code]) + 2;
            char* new_entry = malloc(len);
            snprintf(new_entry, len, "%s%c", dict[prev_code], entry[0]);
            dict[nextCode] = new_entry;
            if (DEBUG) printf("Added to dict: %d → '%s'\n", nextCode, new_entry);
            nextCode++;
        }

        prev_code = code;
    }

    for (int i = 0; i < MAX_DICT_SIZE; i++) {
        free(dict[i]);
    }
}

int main() {
    FILE* in = fopen("incomp", "rb");
    FILE* out = fopen("outdecomp", "wb");
    if (!in || !out) {
        perror("File open failed");
        if (in) fclose(in);
        if (out) fclose(out);
        return 1;
    }

    decompress(in, out);

    fclose(in);
    fclose(out);
    return 0;
}
