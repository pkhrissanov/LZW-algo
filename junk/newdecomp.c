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
        bit_buffer = (bit_buffer << 8) | (uint8_t)byte; // add the unsigned byte to the buffer
        bit_count += 8;
    }
    int shift = bit_count - bits; // how many bytes we added, want to find how much to shift to find bits at the top
    int code  = (bit_buffer >> shift) & ((1 << bits) - 1);
    bit_count -= bits; 
    bit_buffer &= (1u << bit_count) - 1; // gets rid of the old bits
    return code;
}

// Reset dictionary to initial 256 entries (do NOT touch bit_buffer/bit_count)
void dict_reset(int *next_code, int *code_size) {
    for (int i = 0; i < MAX_DICT_SIZE; i++) {
        free(dictionary[i]); // gets rid of each position in array
        dictionary[i] = NULL; //safety precaution
    }
    for (int c = 0; c < 256; c++) { //sets up the intial dictionary
        dictionary[c] = malloc(2);
        dictionary[c][0] = (char)c;
        dictionary[c][1] = '\0';
    }
    *next_code = 256;
    *code_size = INITIAL_BITS; //dict gets reset - dont need to read more than 9 until later
}

char *dupstr(const char *s) { // duplicates the string
    size_t len = strlen(s) + 1;
    char *r = malloc(len);
    memcpy(r, s, len);
    return r;
}

int main() {

    //general housekeeping - input file stuff
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
    dict_reset(&next_code, &code_size); //just as a precauion 

    int prev_code = read_bits(in, code_size); //reads first code 
    if (prev_code < 0 || prev_code >= MAX_DICT_SIZE || !dictionary[prev_code]) { //checks to see if the first code actually exists
        fprintf(stderr, "Invalid first code: %d\n", prev_code);
        return 1;
    }
    char *prev_entry = dupstr(dictionary[prev_code]);
    fwrite(prev_entry, 1, strlen(prev_entry), out);

    while (1) {
        // DEBUG: show state before reading
        fprintf(stderr,
            "[DEBUG] about to read %d bits | dict size = %d | bit_count = %d | bit_buffer = 0x%X\n",
            code_size, next_code, bit_count, bit_buffer
        );

        int curr_code = read_bits(in, code_size); // starts to read the code
        fprintf(stderr, "[DEBUG] got code = %d\n", curr_code);

        if (curr_code < 0 || curr_code == END_MARKER) break; //if broken or is at end, stop

        if (curr_code == RESET_MARKER) { //logic for if hits reset
            dict_reset(&next_code, &code_size); //resets dictionary
            free(prev_entry);
            prev_code = read_bits(in, code_size); //same logic as above, if less than 0 or not intialized, break 
            if (prev_code < 0 || !dictionary[prev_code]) {
                fprintf(stderr, "Invalid code after reset: %d\n", prev_code);
                break;
            }
            prev_entry = dupstr(dictionary[prev_code]);
            fwrite(prev_entry, 1, strlen(prev_entry), out);
            continue;
        }

        char *entry;
        if (curr_code < next_code && dictionary[curr_code]) { ///if current code is in dict, duplicate 
            entry = dupstr(dictionary[curr_code]);
        } else if (curr_code == next_code && prev_entry) { 
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


            if (next_code > ((1u << code_size) - 1) && code_size < MAX_BITS) {
                code_size++;
                fprintf(stderr, "[DEBUG] bumped code_size to %d\n", code_size);
            }
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
