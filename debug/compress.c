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
#define BIT_BUMP_MARKER 4096  // NEW: special marker to indicate a bit size increase

uint64_t bit_buffer = 0;
int bit_count = 0;


unsigned long hash(unsigned char *str) {
    unsigned long h = 5381;
    int c;
    while ((c = *str++)) h = ((h << 5) + h) + c;
    return h;
}

typedef struct node {
    char word[WORD_LEN];
    unsigned long hashValue;
    int index;
    struct node* next;
} node;

node* dict_by_index[MAX_DICT_SIZE];


node* create_node(const char* word, int code) {
    node* n = malloc(sizeof(node));
    if (!n) { perror("malloc failed for new node"); exit(1); }
    strncpy(n->word, word, WORD_LEN - 1);
    n->word[WORD_LEN - 1] = '\0';
    n->hashValue = hash((unsigned char*)word);
    n->index = code;
    n->next = NULL;
    return n;
}

bool lookup(const char* key, node* ht[], int size) {
    unsigned long h = hash((unsigned char*)key) % size;
    for (node* cur = ht[h]; cur; cur = cur->next)
        if (strcmp(cur->word, key) == 0) return true;
    return false;
}

// === PATCHED: Also store pointer by index ===
void insert_to_dict(node*** htPtr, const char* word, int code, int* count) {
    node** ht = *htPtr;
    node* n = create_node(word, code);
    int idx = n->hashValue % TABLE_SIZE;
    n->next = ht[idx];
    ht[idx] = n;
    dict_by_index[code] = n;
    (*count)++;
}

void write_bits(FILE* o, uint16_t c, int sz) {
    bit_buffer = (bit_buffer << sz) | c;
    bit_count += sz;
    while (bit_count >= 8) {
        uint8_t b = (bit_buffer >> (bit_count - 8)) & 0xFF;
        fputc(b, o);
        bit_count -= 8;
    }
}

void flush_bits(FILE* o) {
    if (bit_count > 0) {
        uint8_t b = (bit_buffer << (8 - bit_count)) & 0xFF;
        fputc(b, o);
    }
    bit_buffer = 0;
    bit_count = 0;
}

void dict_init(node*** htPtr, int* nextCode, int* entryCount) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        node* cur = (*htPtr)[i];
        while (cur) {
            node* temp = cur;
            cur = cur->next;
            free(temp);
        }
        (*htPtr)[i] = NULL;
    }

    *entryCount = 0;
    for (int i = 0; i < 256; i++) {
        char s[2] = {(char)i, '\0'};
        insert_to_dict(htPtr, s, i, entryCount);
    }
    *nextCode = 258;
}

// === NEW: Print dictionary contents by index ===
void print_dict_by_index(int upto) {
    printf("\n--- Final Dictionary ---\n");
    for (int i = 0; i < upto; i++) {
        if (dict_by_index[i]) {
            printf("Index %4d | Code: %4d | Word: '%s'\n", i, dict_by_index[i]->index, dict_by_index[i]->word);
        }
    }
    printf("------------------------\n");
}

void lzw_algo(FILE* in, FILE* out) {
    node** ht = calloc(TABLE_SIZE, sizeof(node*));
    if (!ht) { perror("calloc for hash table failed"); exit(1); }

    int nextCode;
    int entryCount = 0;
    int reset_count = 0;

    dict_init(&ht, &nextCode, &entryCount);

    char* cur = malloc(WORD_LEN);
    char* nxt = malloc(WORD_LEN);
    if (!cur || !nxt) { perror("malloc for word buffers failed"); exit(1); }
    cur[0] = '\0';

    int codeSize = INITIAL_CODE_SIZE;
    int ch;

    while ((ch = fgetc(in)) != EOF) {
        snprintf(nxt, WORD_LEN, "%s%c", cur, (char)ch);

        if (lookup(nxt, ht, TABLE_SIZE)) {
            strcpy(cur, nxt);
        } else {
            int c = 0;
            unsigned long h = hash((unsigned char*)cur) % TABLE_SIZE;
            for (node* x = ht[h]; x; x = x->next) {
                if (!strcmp(x->word, cur)) {
                    c = x->index;
                    break;
                }
            }
            write_bits(out, c, codeSize);
            if (DEBUG) printf("Wrote code %x for '%s' (size %d)\n", c, cur, codeSize);

            if (nextCode < MAX_DICT_SIZE) {
                insert_to_dict(&ht, nxt, nextCode, &entryCount);
                if (DEBUG) printf("Inserted '%s' as code %x\n", nxt, nextCode);

                if ((nextCode + 1) == (1 << codeSize) && codeSize < MAX_BITS) {
                    write_bits(out, BIT_BUMP_MARKER, codeSize);
                    if (DEBUG) printf("Wrote BIT_BUMP_MARKER (0x1000) at codeSize %d\n", codeSize);
                    codeSize++;
                    if (DEBUG) printf("Increased codeSize to %d\n", codeSize);
                }

                nextCode++;
            } else {
                write_bits(out, CLEAR_CODE, codeSize);
                if (DEBUG) printf("Dictionary full. Emitting CLEAR_CODE.\n");
                reset_count++;

                dict_init(&ht, &nextCode, &entryCount);
                codeSize = INITIAL_CODE_SIZE;

                cur[0] = (char)ch;
                cur[1] = '\0';
                continue;
            }

            cur[0] = (char)ch;
            cur[1] = '\0';
        }
    }

    if (cur[0]) {
        unsigned long h = hash((unsigned char*)cur) % TABLE_SIZE;
        int c = 0;
        for (node* x = ht[h]; x; x = x->next) {
            if (!strcmp(x->word, cur)) {
                c = x->index;
                break;
            }
        }
        write_bits(out, c, codeSize);
        if (DEBUG) printf("Wrote last code %d for '%s'\n", c, cur);
    }

    write_bits(out, END_CODE, codeSize);
    flush_bits(out);

    if (DEBUG) print_dict_by_index(nextCode);  // === ADDED ===

    free(cur);
    free(nxt);
    for (int i = 0; i < TABLE_SIZE; i++) {
        node* cur_node = ht[i];
        while (cur_node) {
            node* t = cur_node;
            cur_node = cur_node->next;
            free(t);
        }
    }
    free(ht);

    printf("Compression complete. Dictionary was reset %d times.\n", reset_count);
}

int main() {
    char in_fn[] = "in";
    char out_fn[] = "incomp";

    FILE* in = fopen(in_fn, "rb");
    if (!in) { perror("Failed to open input file 'in'"); return 1; }
    FILE* out = fopen(out_fn, "wb");
    if (!out) { perror("Failed to open output file 'incomp'"); fclose(in); return 1; }

    lzw_algo(in, out);

    fclose(in);
    fclose(out);

    return 0;
}
