#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define DEBUG 1  // Enable debug logging
#define TABLE_SIZE 4096
#define WORD_LEN 100
#define INITIAL_TABLE_SIZE 4096
#define MAX_DICT_SIZE 4096
#define RESET_MARKER 4095
#define END_MARKER 4094

uint32_t bit_buffer = 0;
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

node* create_node(const char* word, int code) {
    node* n = malloc(sizeof(node));
    if (!n) { perror("malloc"); exit(1); }
    strncpy(n->word, word, WORD_LEN-1);
    n->word[WORD_LEN-1] = '\0';
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

void insert_to_dict(node*** htPtr, int** bcPtr, int* size,
                    const char* word, int code, int* count) {
    node** ht = *htPtr;
    int* bc = *bcPtr;
    if (lookup(word, ht, *size)) return;
    node* n = create_node(word, code);
    int idx = n->hashValue % *size;
    n->next = ht[idx];
    ht[idx] = n;
    bc[idx]++;
    (*count)++;
}

void write_bits(FILE* o, uint16_t c, int sz) {
    bit_buffer = (bit_buffer << sz) | c;
    bit_count += sz;
    while (bit_count >= 8) {
        uint8_t b = (bit_buffer >> (bit_count-8)) & 0xFF;
        fputc(b, o);
        bit_count -= 8;
    }
}

void flush_bits(FILE* o) {
    if (bit_count > 0) {
        uint8_t b = (bit_buffer << (8-bit_count)) & 0xFF;
        fputc(b, o);
    }
    bit_buffer = bit_count = 0;
}

void dict_init(node*** htPtr, int** bcPtr, int* size,
               int* nextCode, int* entryCount) {
    for (int i = 0; i < 256; i++) {
        char s[2] = {(char)i, '\0'};
        insert_to_dict(htPtr, bcPtr, size, s, i, entryCount);
    }
    *nextCode = 256;
}

// simulate_decompression prototype and implementation
void simulate_decompression(int* codes, int code_count, node** hashTable, int tableSize) {
    printf("\n--- Simulated Decompressed Output ---\n");
    for (int i = 0; i < code_count; i++) {
        int c = codes[i];
        if (c == RESET_MARKER) {
            // reset dictionary
            for (int j = 0; j < tableSize; j++) {
                node* cur = hashTable[j];
                while (cur) { node* t = cur; cur = cur->next; free(t); }
                hashTable[j] = NULL;
            }
            int dummyNext, dummyCount;
            int* bc = calloc(tableSize, sizeof(int));
            dict_init(&hashTable, &bc, &tableSize, &dummyNext, &dummyCount);
            free(bc);
            continue;
        }
        if (c == END_MARKER) break;
        bool found = false;
        for (int j = 0; j < tableSize && !found; j++) {
            for (node* cur = hashTable[j]; cur; cur = cur->next) {
                if (cur->index == c) {
                    printf("%s", cur->word);
                    found = true;
                    break;
                }
            }
        }
        if (!found) printf("[UNKNOWN:%d]", c);
    }
    printf("\n--- End of Simulation ---\n");
}

void lzw_algo(FILE* in, node*** htPtr, int** bcPtr, int* size,
              int* nextCode, FILE* out, int* entryCount,
              int* sim_codes, int* sim_count, bool sim,
              int* reset_count) {
    node** ht = *htPtr;
    int* bc = *bcPtr;
    char cur[WORD_LEN] = "", nxt[WORD_LEN];
    int codeSize = 9, ch;
    *sim_count = 0;

    while ((ch = fgetc(in)) != EOF) {
        int len = strlen(cur);
        memcpy(nxt, cur, len);
        nxt[len] = (char)ch; nxt[len+1] = '\0';

        if (lookup(nxt, ht, *size)) {
            strcpy(cur, nxt);
        } else {
            if (cur[0]) {
                int c = 0;
                unsigned long h = hash((unsigned char*)cur) % *size;
                for (node* x = ht[h]; x; x = x->next)
                    if (!strcmp(x->word, cur)) { c = x->index; break; }
                write_bits(out, c, codeSize);
                if (sim) sim_codes[(*sim_count)++] = c;
                if (DEBUG) printf("Wrote code %d\n", c);
            }

            if (*nextCode < MAX_DICT_SIZE) {
                insert_to_dict(htPtr, bcPtr, size, nxt, *nextCode, entryCount);
                if (DEBUG) printf("Inserted '%s' as code %d (codeSize=%d)\n", nxt, *nextCode, codeSize);
                (*nextCode)++;
                if (*nextCode == 512) codeSize = 10;
                else if (*nextCode == 1024) codeSize = 11;
                else if (*nextCode == 2048) codeSize = 12;
            } else {
                (*reset_count)++;
                flush_bits(out);
                write_bits(out, RESET_MARKER, codeSize);
                if (sim) sim_codes[(*sim_count)++] = RESET_MARKER;
                if (DEBUG) printf("Dictionary full—reset at code %d\n", *nextCode);

                for (int i = 0; i < *size; i++) {
                    node* x = ht[i]; while (x) { node* t = x; x = x->next; free(t);} ht[i] = NULL; bc[i] = 0;
                }
                *entryCount = 0;
                dict_init(htPtr, bcPtr, size, nextCode, entryCount);
                codeSize = 9;
            }
            cur[0] = (char)ch; cur[1] = '\0';
        }
    }

    if (cur[0]) {
        unsigned long h = hash((unsigned char*)cur) % *size;
        int c = 0;
        for (node* x = ht[h]; x; x = x->next) if (!strcmp(x->word, cur)) { c = x->index; break; }
        write_bits(out, c, codeSize);
        if (sim) sim_codes[(*sim_count)++] = c;
        if (DEBUG) printf("Wrote last code %d\n", c);
    }

    flush_bits(out);
    write_bits(out, END_MARKER, codeSize);
    if (sim) sim_codes[(*sim_count)++] = END_MARKER;
    if (DEBUG) printf("Wrote END_MARKER %d\n", END_MARKER);
}

int main() {
    char in_fn[256], out_fn[256], sim_ch;
    printf("Enter file to compress: "); scanf("%255s", in_fn);
    printf("Enter output file: "); scanf("%255s", out_fn);
    printf("Simulate decompression? (y/n): "); scanf(" %c", &sim_ch);
    bool sim = (sim_ch=='y'||sim_ch=='Y');

    FILE* in = fopen(in_fn, "rb"); if (!in) { perror("in"); return 1; }
    FILE* out = fopen(out_fn, "wb"); if (!out) { perror("out"); return 1; }

    node** ht = calloc(TABLE_SIZE, sizeof(node*));
    int* bc = calloc(TABLE_SIZE, sizeof(int));
    int entries = 0, nextC, simc, resets = 0;
    int sims[10000];

    dict_init(&ht, &bc, &(int){TABLE_SIZE}, &nextC, &entries);
    lzw_algo(in, &ht, &bc, &(int){TABLE_SIZE}, &nextC, out,
             &entries, sims, &simc, sim, &resets);
    fclose(in);
    fclose(out);

    if (sim) simulate_decompression(sims, simc, ht, TABLE_SIZE);
    printf("Dictionary was reset %d times.\n", resets);

    for (int i = 0; i < TABLE_SIZE; i++) {
        node* cur = ht[i];
        while (cur) { node* t = cur; cur = cur->next; free(t); }
    }
    free(ht);
    free(bc);
    return 0;
}
