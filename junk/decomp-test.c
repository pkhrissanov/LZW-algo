#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_DICT_SIZE 4096
#define RESET_MARKER 4095
#define END_MARKER 4094

static uint32_t buffer = 0;
static int bits = 0;

int read_bits(FILE *in, int size) {
    while (bits < size) {
        int byte = fgetc(in);
        if (byte == EOF) return -1;
        buffer = (buffer << 8) | (uint8_t)byte;
        bits += 8;
    }
    int shift = bits - size;
    int code = (buffer >> shift) & ((1 << size) - 1);
    bits -= size;
    buffer &= (1U << bits) - 1;
    return code;
}

char *dict[MAX_DICT_SIZE];

void reset_dict(int *next_code, int *code_size) {
    for (int i = 0; i < MAX_DICT_SIZE; i++) {
        free(dict[i]);
        dict[i] = NULL;
    }
    for (int i = 0; i < 256; i++) {
        dict[i] = malloc(2);
        dict[i][0] = (char)i;
        dict[i][1] = '\0';
    }
    *next_code = 256;
    *code_size = 9;
    buffer = 0;
    bits = 0;
}

int main() {
    char in_file[256], out_file[256];
    printf("Enter compressed file name: ");
    scanf("%255s", in_file);
    printf("Enter output file name: ");
    scanf("%255s", out_file);
    FILE *fin = fopen(in_file, "rb");
    FILE *fout = fopen(out_file, "wb");
    if (!fin || !fout) return 1;

    int code_size, next_code;
    reset_dict(&next_code, &code_size);

    int prev_code = read_bits(fin, code_size);
    if (prev_code < 0 || prev_code >= MAX_DICT_SIZE || !dict[prev_code]) return 1;
    fputs(dict[prev_code], fout);
    char *prev_entry = strdup(dict[prev_code]);

    while (1) {
        int code = read_bits(fin, code_size);
        if (code == -1 || code == END_MARKER) break;
        if (code == RESET_MARKER) {
            reset_dict(&next_code, &code_size);
            prev_code = read_bits(fin, code_size);
            if (prev_code < 0 || !dict[prev_code]) break;
            free(prev_entry);
            prev_entry = strdup(dict[prev_code]);
            fputs(prev_entry, fout);
            continue;
        }
        char *entry;
        if (code < next_code && dict[code]) {
            entry = strdup(dict[code]);
        } else {
            int len = strlen(prev_entry);
            entry = malloc(len + 2);
            memcpy(entry, prev_entry, len);
            entry[len] = prev_entry[0];
            entry[len + 1] = '\0';
        }
        fputs(entry, fout);

        if (next_code < MAX_DICT_SIZE) {
            int len = strlen(prev_entry);
            dict[next_code] = malloc(len + 2);
            memcpy(dict[next_code], prev_entry, len);
            dict[next_code][len] = entry[0];
            dict[next_code][len + 1] = '\0';
            next_code++;
            if (next_code == 512) code_size = 10;
            else if (next_code == 1024) code_size = 11;
            else if (next_code == 2048) code_size = 12;
        }
        free(prev_entry);
        prev_entry = entry;
        prev_code = code;
    }

    free(prev_entry);
    for (int i = 0; i < MAX_DICT_SIZE; i++) free(dict[i]);
    fclose(fin);
    fclose(fout);
    return 0;
}
