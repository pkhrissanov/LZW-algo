#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_DICT_SIZE 4096
#define WORD_LEN 256
#define RESET_MARKER 4095
#define END_MARKER 4094

uint32_t bit_buffer = 0;
int bit_count = 0;

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

void lzw_decompress(FILE *in, FILE *out) {
    char* dict[MAX_DICT_SIZE];
    int next_code = 256;
    int code_size = 9;

    for (int i = 0; i < 256; i++) {
        dict[i] = malloc(2);
        dict[i][0] = (char)i;
        dict[i][1] = '\0';
    }

    int prev_code = read_bits(in, code_size);
    if (prev_code == -1) return;

    fputs(dict[prev_code], out);
    char prev_str[WORD_LEN];
    strcpy(prev_str, dict[prev_code]);

    while (1) {
        int code = read_bits(in, code_size);
        if (code == -1) break;
        if (code == END_MARKER) break;

        char entry[WORD_LEN];

        if (code < next_code) {
            strcpy(entry, dict[code]);
        } else {
            snprintf(entry, WORD_LEN, "%s%c", prev_str, prev_str[0]);
        }

        fputs(entry, out);

        if (next_code < MAX_DICT_SIZE) {
            snprintf(dict[next_code] = malloc(strlen(prev_str) + 2), WORD_LEN, "%s%c", prev_str, entry[0]);
            next_code++;
            if (next_code >= (1 << code_size) && code_size < 12) code_size++;
        }

        strcpy(prev_str, entry);
    }

    for (int i = 0; i < next_code; i++) free(dict[i]);
}

int main() {
    char in_name[256], out_name[256];
    printf("Compressed input file: ");
    scanf("%255s", in_name);
    printf("Decompressed output file: ");
    scanf("%255s", out_name);

    FILE* in = fopen(in_name, "rb");
    FILE* out = fopen(out_name, "w");

    if (!in || !out) {
        perror("Failed to open files");
        return 1;
    }

    lzw_decompress(in, out);
    fclose(in);
    fclose(out);
    return 0;
}
