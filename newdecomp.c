#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_DICT_SIZE 4096
#define INITIAL_BITS   9
#define MAX_BITS      12
#define RESET_MARKER 4095
#define END_MARKER   4094

static uint32_t bit_buffer = 0;
static int      bit_count  = 0;
static char*   dictionary[MAX_DICT_SIZE];

// Read 'bits' bits from 'in', return code or -1 on EOF
int read_bits(FILE *in, int bits) {
    while (bit_count < bits) {
        int byte = fgetc(in);
        if (byte == EOF) return -1;
        bit_buffer = (bit_buffer << 8) | (uint8_t)byte;
        bit_count += 8;
    }
    int shift = bit_count - bits;
    int code  = (bit_buffer >> shift) & ((1 << bits) - 1);
    bit_count -= bits;
    bit_buffer &= (1u << bit_count) - 1;
    return code;
}

// Reset dictionary to initial 256 entries
void dict_reset(int *next_code, int *code_size) {
    for (int i = 0; i < MAX_DICT_SIZE; i++) {
        free(dictionary[i]);
        dictionary[i] = NULL;
    }
    for (int c = 0; c < 256; c++) {
        dictionary[c] = malloc(2);
        dictionary[c][0] = (char)c;
        dictionary[c][1] = '\0';
    }
    *next_code = 256;
    *code_size = INITIAL_BITS;
    bit_buffer = 0;
    bit_count  = 0;
}

char *dupstr(const char *s) {
    size_t len = strlen(s) + 1;
    char *r = malloc(len);
    memcpy(r, s, len);
    return r;
}

int main() {
    char in_name[256], out_name[256];
    printf("Enter compressed file name: ");
    if (scanf("%255s", in_name) != 1) return 1;
    printf("Enter output file name: ");
    if (scanf("%255s", out_name) != 1) return 1;

    FILE *in  = fopen(in_name,  "rb");
    FILE *out = fopen(out_name, "wb");
    if (!in || !out) {
        perror("File open failed");
        return 1;
    }

    int code_size, next_code;
    dict_reset(&next_code, &code_size);

    int prev_code = read_bits(in, code_size);
    if (prev_code < 0 || prev_code >= MAX_DICT_SIZE || !dictionary[prev_code]) {
        fprintf(stderr, "Invalid first code: %d\n", prev_code);
        return 1;
    }
    char *prev_entry = dupstr(dictionary[prev_code]);
    fwrite(prev_entry, 1, strlen(prev_entry), out);

    while (1) {
        // Bump code_size BEFORE reading next code
        if      (next_code >= (1 << (MAX_BITS - 1))) code_size = MAX_BITS;
        else if (next_code >= (1 << 10))             code_size = 11;
        else if (next_code >= (1 << 9))              code_size = 10;

        int curr_code = read_bits(in, code_size);
        if (curr_code < 0 || curr_code == END_MARKER) break;

        if (curr_code == RESET_MARKER) {
            dict_reset(&next_code, &code_size);
            free(prev_entry);
            prev_code = read_bits(in, code_size);
            if (prev_code < 0 || !dictionary[prev_code]) {
                fprintf(stderr, "Invalid code after reset: %d\n", prev_code);
                break;
            }
            prev_entry = dupstr(dictionary[prev_code]);
            fwrite(prev_entry, 1, strlen(prev_entry), out);
            continue;
        }

        char *entry;
        if (curr_code < next_code && dictionary[curr_code]) {
            entry = dupstr(dictionary[curr_code]);
        } else if (curr_code == next_code && dictionary[prev_code]) {
            size_t L = strlen(prev_entry);
            entry = malloc(L + 2);
            memcpy(entry, prev_entry, L);
            entry[L]     = prev_entry[0];
            entry[L+1]   = '\0';
        } else {
            fprintf(stderr, "Unexpected code: %d\n", curr_code);
            break;
        }

        fwrite(entry, 1, strlen(entry), out);

        if (next_code < MAX_DICT_SIZE) {
            size_t L = strlen(prev_entry);
            char *new_entry = malloc(L + 2);
            memcpy(new_entry, prev_entry, L);
            new_entry[L]   = entry[0];
            new_entry[L+1] = '\0';
            dictionary[next_code++] = new_entry;
        }

        free(prev_entry);
        prev_entry = entry;
        prev_code  = curr_code;
    }

    free(prev_entry);
    for (int i = 0; i < MAX_DICT_SIZE; i++) free(dictionary[i]);
    fclose(in);
    fclose(out);
    return 0;
}
