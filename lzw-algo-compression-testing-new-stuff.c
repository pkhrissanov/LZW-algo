#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define TABLE_SIZE 4096
#define WORD_LEN 256
#define DEBUG false
#define MAX_DICT_SIZE 4096
#define RESET_MARKER 4095
#define END_MARKER 4094

static uint32_t buffer = 0;
static int bits_in_buffer = 0;

void write_bits(FILE *out, uint16_t code, int code_size) {
    buffer = (buffer << code_size) | code;
    bits_in_buffer += code_size;

    while (bits_in_buffer >= 8) {
        uint8_t byte = (buffer >> (bits_in_buffer - 8)) & 0xFF;
        fputc(byte, out);
        bits_in_buffer -= 8;
    }
}

void flush_bits(FILE *out) {
    if (bits_in_buffer > 0) {
        uint8_t byte = (buffer << (8 - bits_in_buffer)) & 0xFF;
        fputc(byte, out);
    }
    buffer = 0;
    bits_in_buffer = 0;
}

unsigned long hash(unsigned char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

typedef struct node {
    char word[WORD_LEN];
    unsigned long hashValue;
    int index;
    struct node* next;
} node;

node* create_node(const char* word, int code) {
    node* new_node = malloc(sizeof(node));
    if (!new_node) {
        perror("Memory allocation failed");
        exit(1);
    }
    strncpy(new_node->word, word, WORD_LEN - 1);
    new_node->word[WORD_LEN - 1] = '\0';
    new_node->hashValue = hash((unsigned char*)word);
    new_node->index = code;
    new_node->next = NULL;
    return new_node;
}

void lzw_compress(FILE *in, FILE *out) {
    node* table[TABLE_SIZE] = {NULL};
    int code_size = 9;
    int next_code = 256;

    for (int i = 0; i < 256; i++) {
        char str[2] = {i, '\0'};
        unsigned long h = hash((unsigned char*)str) % TABLE_SIZE;
        node* entry = create_node(str, i);
        entry->next = table[h];
        table[h] = entry;
    }

    char current[WORD_LEN] = "";
    int c;
    while ((c = fgetc(in)) != EOF) {
        char next[WORD_LEN];
        if (snprintf(next, WORD_LEN, "%s%c", current, c) >= WORD_LEN)
            continue; // Skip too-long strings

        unsigned long h = hash((unsigned char*)next) % TABLE_SIZE;
        node* n = table[h];
        while (n && strcmp(n->word, next) != 0) n = n->next;

        if (n) {
            strncpy(current, next, WORD_LEN);
        } else {
            unsigned long hc = hash((unsigned char*)current) % TABLE_SIZE;
            node* m = table[hc];
            while (m && strcmp(m->word, current) != 0) m = m->next;
            if (m) write_bits(out, m->index, code_size);

            if (next_code < MAX_DICT_SIZE) {
                node* entry = create_node(next, next_code++);
                entry->next = table[h];
                table[h] = entry;
                if (next_code >= (1 << code_size) && code_size < 12) code_size++;
            }

            current[0] = c;
            current[1] = '\0';
        }
    }

    if (current[0]) {
        unsigned long hc = hash((unsigned char*)current) % TABLE_SIZE;
        node* m = table[hc];
        while (m && strcmp(m->word, current) != 0) m = m->next;
        if (m) write_bits(out, m->index, code_size);
    }

    write_bits(out, END_MARKER, code_size);
    flush_bits(out);
}

int main() {
    char in_name[256], out_name[256];
    printf("Input file: ");
    scanf("%255s", in_name);
    printf("Output file: ");
    scanf("%255s", out_name);

    FILE* in = fopen(in_name, "rb");
    FILE* out = fopen(out_name, "wb");

    if (!in || !out) {
        perror("Failed to open files");
        return 1;
    }

    lzw_compress(in, out);
    fclose(in);
    fclose(out);
    return 0;
}
