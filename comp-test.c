#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>


#define MAX_DICT_SIZE 4096
#define INITIAL_TABLE_SIZE 4096
#define WORD_LEN 100
#define RESET_MARKER 4095
#define END_MARKER 4094


typedef struct node {
    char word[WORD_LEN];
    int code;
    struct node* next;
} node;

static node** hashTable = NULL;
static int* bucketCounts = NULL;
static int tableSize = INITIAL_TABLE_SIZE;
static int entryCount = 0;
static int nextCode = 0;


static uint32_t bit_buffer = 0;
static int bit_count = 0;


static unsigned long hash_func(const char* str) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}


static node* create_node(const char* word, int code) {
    node* new_node = malloc(sizeof(node));
    if (!new_node) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    strncpy(new_node->word, word, WORD_LEN - 1);
    new_node->word[WORD_LEN - 1] = '\0';
    new_node->code = code;
    new_node->next = NULL;
    return new_node;
}


static bool lookup(const char* key) {
    unsigned long idx = hash_func(key) % tableSize;
    for (node* cur = hashTable[idx]; cur; cur = cur->next) {
        if (strcmp(cur->word, key) == 0) return true;
    }
    return false;
}


static int get_code(const char* key) {
    unsigned long idx = hash_func(key) % tableSize;
    for (node* cur = hashTable[idx]; cur; cur = cur->next) {
        if (strcmp(cur->word, key) == 0) return cur->code;
    }
    fprintf(stderr, "Error: key '%s' not found in dictionary.\n", key);
    exit(EXIT_FAILURE);
}


static void insert_to_dict(const char* word, int code) {
    if (entryCount >= MAX_DICT_SIZE || lookup(word)) return;
    unsigned long idx = hash_func(word) % tableSize;
    node* new_node = create_node(word, code);
    new_node->next = hashTable[idx];
    hashTable[idx] = new_node;
    bucketCounts[idx]++;
    entryCount++;
}


static void dict_init() {
    // Clear existing entries
    for (int i = 0; i < tableSize; i++) {
        node* cur = hashTable[i];
        while (cur) {
            node* tmp = cur;
            cur = cur->next;
            free(tmp);
        }
        hashTable[i] = NULL;
        bucketCounts[i] = 0;
    }
    entryCount = 0;
    nextCode = 256;
    // Populate initial dictionary
    for (int i = 0; i < 256; i++) {
        char s[2] = {(char)i, '\0'};
        insert_to_dict(s, i);
    }
}


static void write_bits(FILE* out, uint16_t code, int code_size) {
    bit_buffer = (bit_buffer << code_size) | code;
    bit_count += code_size;
    while (bit_count >= 8) {
        uint8_t byte = (bit_buffer >> (bit_count - 8)) & 0xFF;
        fputc(byte, out);
        bit_count -= 8;
    }
}


static void flush_bits(FILE* out) {
    if (bit_count > 0) {
        uint8_t byte = (bit_buffer << (8 - bit_count)) & 0xFF;
        fputc(byte, out);
    }
    bit_buffer = 0;
    bit_count = 0;
}

int main() {
    char in_name[256], out_name[256];
    printf("Enter input file: ");
    scanf("%255s", in_name);
    FILE* input = fopen(in_name, "rb");
    if (!input) { perror("fopen input"); return 1; }

    printf("Enter output file: ");
    scanf("%255s", out_name);
    FILE* output = fopen(out_name, "wb");
    if (!output) { perror("fopen output"); fclose(input); return 1; }

    // Allocate hash table and bucket counts
    hashTable = calloc(tableSize, sizeof(node*));
    bucketCounts = calloc(tableSize, sizeof(int));
    if (!hashTable || !bucketCounts) { perror("calloc"); return 1; }

    dict_init();

    char current[WORD_LEN] = "";
    int code_size = 9;
    int c;

    while ((c = fgetc(input)) != EOF) {
        char next[WORD_LEN];
        strcpy(next, current);
        size_t len = strlen(next);
        if (len + 1 < WORD_LEN) {
            next[len] = (char)c;
            next[len+1] = '\0';
        }
        
        if (lookup(next)) {
            strcpy(current, next);
        } else {

            int code = get_code(current);
            write_bits(output, code, code_size);
        
        
            if (nextCode < MAX_DICT_SIZE) {
                insert_to_dict(next, nextCode++);
                if (nextCode == 512) code_size = 10;
                else if (nextCode == 1024) code_size = 11;
                else if (nextCode == 2048) code_size = 12;
            } else {

                flush_bits(output);
                write_bits(output, RESET_MARKER, code_size);
                dict_init();
                code_size = 9;
            }
            current[0] = (char)c;
            current[1] = '\0';
        }
    }


    if (strlen(current) > 0) {
        int code = get_code(current);
        write_bits(output, code, code_size);
    }


    flush_bits(output);
    write_bits(output, END_MARKER, code_size);
    flush_bits(output);

    fclose(input);
    fclose(output);


    for (int i = 0; i < tableSize; i++) {
        node* cur = hashTable[i];
        while (cur) {
            node* tmp = cur;
            cur = cur->next;
            free(tmp);
        }
    }
    free(hashTable);
    free(bucketCounts);

    return 0;
}
