#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define DEBUG 1
#define TABLE_SIZE 8192
#define WORD_LEN 8192
#define MAX_DICT_SIZE 8192
#define CLEAR_CODE 256
#define END_CODE 257
#define INITIAL_CODE_SIZE 10
#define MAX_BITS 13
#define BIT_BUMP_MARKER 4096

uint64_t bit_buffer = 0;
int bit_count = 0;
int codeSize = INITIAL_CODE_SIZE;

typedef struct node {
    char word[WORD_LEN];
    int index;
    struct node* next;
} node;

node* dict_by_index[MAX_DICT_SIZE];

unsigned long hash(unsigned char *str) {
    unsigned long h = 5381;
    int c;
    while ((c = *str++)) h = ((h << 5) + h) + c;
    return h;
}

node* create_node(const char* word, int code) {
    node* n = malloc(sizeof(node));
    if (!n) { perror("malloc failed"); exit(1); }
    strncpy(n->word, word, WORD_LEN - 1);
    n->word[WORD_LEN - 1] = '\0';
    n->index = code;
    n->next = NULL;
    return n;
}

void insert_to_dict(node*** htPtr, const char* word, int code) {
    node** ht = *htPtr;
    node* n = create_node(word, code);
    int idx = hash((unsigned char*)word) % TABLE_SIZE;
    n->next = ht[idx];
    ht[idx] = n;
    dict_by_index[code] = n;
}

node* find_by_index(node** ht, int index) {
    if (index >= 0 && index < MAX_DICT_SIZE && dict_by_index[index]) {
        return dict_by_index[index];
    }
    return NULL;
}

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

    if (DEBUG) printf("[DEBUG] Read code = %d (size %d)\n", code, sz);
    return code;
}

void dict_init(node*** htPtr, int* nextCode) {
    node** ht = *htPtr;
    for (int i = 0; i < TABLE_SIZE; i++) {
        node* cur = ht[i];
        while (cur) {
            node* tmp = cur;
            cur = cur->next;
            free(tmp);
        }
        ht[i] = NULL;
    }
    for (int i = 0; i < 256; i++) {
        char s[2] = { (char)i, '\0' };
        insert_to_dict(htPtr, s, i);
    }
    *nextCode = 258;
}

void dump_dict_to_file(const char* filename, int upto) {
    FILE* f = fopen(filename, "w");
    if (!f) { perror("Failed to open dictionary dump file"); return; }
    fprintf(f, "--- Final Dictionary ---\n");
    for (int i = 0; i < upto; i++) {
        if (dict_by_index[i]) {
            fprintf(f, "Index %4d | Code: %4d | Word: '%s'\n", i, dict_by_index[i]->index, dict_by_index[i]->word);
        }
    }
    fprintf(f, "------------------------\n");
    fclose(f);
}

void decompress(FILE* in, FILE* out) {
    node** ht = calloc(TABLE_SIZE, sizeof(node*));
    if (!ht) { perror("calloc failed"); exit(1); }

    int nextCode;
    dict_init(&ht, &nextCode);

    int code = read_bits(in, codeSize);
    while (code == BIT_BUMP_MARKER) {
        codeSize++;
        if (DEBUG) printf("[DEBUG] Bump codeSize to %d\n", codeSize);
        code = read_bits(in, codeSize);
    }

    if (code < 0) return;

    node* prev = find_by_index(ht, code);
    if (!prev) {
        fprintf(stderr, "Invalid initial code %d\n", code);
        dump_dict_to_file("decompress_dict", nextCode);
        return;
    }
    fputs(prev->word, out);

    while (1) {
        code = read_bits(in, codeSize);
        while (code == BIT_BUMP_MARKER) {
            codeSize++;
            if (DEBUG) fprintf(stderr, "[DEBUG] BIT_BUMP_MARKER seen, codeSize increased to %d\n", codeSize);
            code = read_bits(in, codeSize);
        }

        if (DEBUG) fprintf(stderr, "[DEBUG] Got code = %d\n", code);
        if (code == END_CODE) break;

        if (code == CLEAR_CODE) {
            if (DEBUG) printf("[DEBUG] CLEAR_CODE received\n");
            dict_init(&ht, &nextCode);
            codeSize = INITIAL_CODE_SIZE;

            code = read_bits(in, codeSize);
            while (code == BIT_BUMP_MARKER) {
                codeSize++;
                if (DEBUG) printf("[DEBUG] Bump codeSize to %d\n", codeSize);
                code = read_bits(in, codeSize);
            }

            if (code < 0) return;
            prev = find_by_index(ht, code);
            if (!prev) {
                fprintf(stderr, "Invalid code after clear: %d\n", code);
                dump_dict_to_file("decompress_dict", nextCode);
                return;
            }
            fputs(prev->word, out);
            continue;
        }

        char entry[WORD_LEN];
        node* entryNode = find_by_index(ht, code);

        if (entryNode) {
            strncpy(entry, entryNode->word, WORD_LEN - 1);
            entry[WORD_LEN - 1] = '\0';
        } else if (code == nextCode) {
            size_t len = strnlen(prev->word, WORD_LEN - 1);
            if (len < WORD_LEN - 1) {
                memcpy(entry, prev->word, len);
                entry[len] = prev->word[0];
                entry[len + 1] = '\0';
            } else {
                fprintf(stderr, "Entry buffer too small\n");
                return;
            }
            if (DEBUG) printf("[DEBUG] Special case built: %s\n", entry);
        } else {
            fprintf(stderr, "Invalid code: %d (nextCode = %d, codeSize = %d)\n", code, nextCode, codeSize);
            dump_dict_to_file("decompress_dict", nextCode);
            return;
        }

        fputs(entry, out);

        if (nextCode < MAX_DICT_SIZE) {
            char new_entry[WORD_LEN];
            size_t len = strnlen(prev->word, WORD_LEN - 1);
            if (len < WORD_LEN - 1) {
                memcpy(new_entry, prev->word, len);
                new_entry[len] = entry[0];
                new_entry[len + 1] = '\0';
                insert_to_dict(&ht, new_entry, nextCode++);
                if (DEBUG) printf("Inserted '%s' as code %d\n", new_entry, nextCode - 1);

                if (nextCode == (1 << codeSize) && codeSize < MAX_BITS) {
                    codeSize++;
                    if (DEBUG) printf("[DEBUG] Increased codeSize to %d due to full dictionary\n", codeSize);
                }
            } else {
                fprintf(stderr, "new_entry buffer too small\n");
                return;
            }
        }

        prev = find_by_index(ht, code);
    }

    dump_dict_to_file("decompress_dict", nextCode);

    for (int i = 0; i < TABLE_SIZE; i++) {
        node* cur = ht[i];
        while (cur) {
            node* tmp = cur;
            cur = cur->next;
            free(tmp);
        }
    }
    free(ht);
    if (DEBUG) printf("[DEBUG] Decompression done.\n");
}

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    FILE* in = fopen("incomp", "rb");
    FILE* out = fopen("outdecomp", "wb");
    if (!in || !out) {
        perror("File open failed");
        return 1;
    }

    decompress(in, out);

    fclose(in);
    fclose(out);
    return 0;
}
