#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define DEBUG 1  // <-- enable debug prints
#define TABLE_SIZE 4096
#define WORD_LEN 100
#define MAX_DICT_SIZE 4096
#define BIT_BUMP_MARKER 4095  // bit size increase marker
#define INITIAL_CODE_SIZE 8
#define MAX_BITS 12

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

void insert_to_dict(node*** htPtr, const char* word, int code) {
    node** ht = *htPtr;
    node* n = create_node(word, code);
    int idx = n->hashValue % TABLE_SIZE;
    n->next = ht[idx];
    ht[idx] = n;
    if (DEBUG) printf("Inserted '%s' as code %d\n", word, code);
}

bool lookup(const char* key, node* ht[], int* code_out) {
    unsigned long h = hash((unsigned char*)key) % TABLE_SIZE;
    for (node* cur = ht[h]; cur; cur = cur->next) {
        if (strcmp(cur->word, key) == 0) {
            *code_out = cur->index;
            return true;
        }
    }
    return false;
}

void write_bits(FILE* o, uint32_t c, int sz) {
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
    *nextCode = 256;
    for (int i = 0; i < 256; i++) {
        char s[2] = {(char)i, '\0'};
        insert_to_dict(htPtr, s, i);
    }
}

void lzw_algo(FILE* in, FILE* out) {
    node** ht = calloc(TABLE_SIZE, sizeof(node*));
    if (!ht) { perror("calloc for hash table failed"); exit(1); }

    int nextCode;
    dict_init(&ht, &nextCode);

    char* cur = malloc(WORD_LEN);
    char* nxt = malloc(WORD_LEN);
    if (!cur || !nxt) { perror("malloc for word buffers failed"); exit(1); }

    cur[0] = '\0';
    int ch;

    int codeSize = INITIAL_CODE_SIZE;
    int threshold = 1 << codeSize;

    while ((ch = fgetc(in)) != EOF) {
        snprintf(nxt, WORD_LEN, "%s%c", cur, (char)ch);

        int code;
        if (lookup(nxt, ht, &code)) {
            strcpy(cur, nxt);
        } else {
            if (*cur) {
                lookup(cur, ht, &code);
                write_bits(out, code, codeSize);
                if (DEBUG) printf("Wrote code %x for '%s' (codeSize=%d)\n", code, cur, codeSize);
            }

            if (nextCode < MAX_DICT_SIZE) {
                insert_to_dict(&ht, nxt, nextCode++);
            }

            if (nextCode == threshold && codeSize < MAX_BITS) {
                write_bits(out, BIT_BUMP_MARKER, codeSize);
                codeSize++;
                threshold = 1 << codeSize;
                if (DEBUG) printf("Bumped codeSize to %d with marker %d\n", codeSize, BIT_BUMP_MARKER);
            }

            cur[0] = (char)ch;
            cur[1] = '\0';
        }
    }

    if (cur[0]) {
        int code;
        if (lookup(cur, ht, &code)) {
            write_bits(out, code, codeSize);
            if (DEBUG) printf("Final write: code %x for '%s' (codeSize=%d)\n", code, cur, codeSize);
        }
    }

    flush_bits(out);
    free(cur);
    free(nxt);

    for (int i = 0; i < TABLE_SIZE; i++) {
        node* cur = ht[i];
        while (cur) {
            node* t = cur;
            cur = cur->next;
            free(t);
        }
    }
    free(ht);

    printf("Compression complete.\n");
}

int main() {
    FILE* in = fopen("in", "rb");
    FILE* out = fopen("incomp", "wb");
    if (!in || !out) {
        perror("File open failed");
        if (in) fclose(in);
        if (out) fclose(out);
        return 1;
    }

    lzw_algo(in, out);

    fclose(in);
    fclose(out);
    return 0;
}
