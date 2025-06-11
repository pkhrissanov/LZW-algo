#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define TABLE_SIZE 4096
#define WORD_LEN 100
#define DEBUG false
#define INITIAL_TABLE_SIZE 4096
#define LOAD_FACTOR_THRESHOLD 0.75
#define MAX_DICT_SIZE 4096
#define RESET_MARKER 4095
#define END_MARKER 4094

void write_bits(FILE *out, uint16_t code, int code_size);
void flush_bits(FILE *out);

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

bool lookup(const char* key, node* hashTable[], int tableSize) {
    unsigned long h = hash((unsigned char*)key);
    int index = h % tableSize;
    node* current = hashTable[index];
    while (current) {
        if (strcmp(current->word, key) == 0) {
            return true;
        }
        current = current->next;
    }
    return false;
}

void insert_to_dict(node*** hashTablePtr, int** bucketCountsPtr, int* tableSize, const char* word, int code, int* entryCount) {
    if (lookup(word, *hashTablePtr, *tableSize)) return;
    node** hashTable = *hashTablePtr;
    int* bucketCounts = *bucketCountsPtr;
    node* new_node = create_node(word, code);
    int index = new_node->hashValue % *tableSize;
    new_node->next = hashTable[index];
    hashTable[index] = new_node;
    bucketCounts[index]++;
    (*entryCount)++;
}

int get_code(const char* key, node* hashTable[], int tableSize) {
    unsigned long h = hash((unsigned char*)key);
    int index = h % tableSize;
    node* current = hashTable[index];
    while (current) {
        if (strcmp(current->word, key) == 0) {
            return current->index;
        }
        current = current->next;
    }
    fprintf(stderr, "Error: key '%s' not found in dictionary.\n", key);
    exit(1);
}

void dict_init(node*** hashTable, int** bucketCounts, int* tableSize, int* nextCode, int* entryCount) {
    for (int i = 0; i < 256; i++) {
        char str[2] = {(char)i, '\0'};
        insert_to_dict(hashTable, bucketCounts, tableSize, str, i, entryCount);
    }
    *nextCode = 256;
}

void lzw_algo(FILE* input, node*** hashTablePtr, int** bucketCountsPtr, int* tableSize,
              int* nextCode, FILE* output, int* entryCount) {
    node** hashTable = *hashTablePtr;
    int* bucketCounts = *bucketCountsPtr;
    char current_str[WORD_LEN] = "";
    char next_str[WORD_LEN];
    int code_size = 9;
    int c;

    while ((c = fgetc(input)) != EOF) {
        size_t len = strnlen(current_str, WORD_LEN - 1);
        memcpy(next_str, current_str, len);
        next_str[len] = (char)c;
        next_str[len + 1] = '\0';

        if (lookup(next_str, hashTable, *tableSize)) {
            strcpy(current_str, next_str);
        } else {
            if (strlen(current_str) > 0) {
                int code = get_code(current_str, hashTable, *tableSize);
                printf("Writing code: %d (nextCode: %d, codeSize: %d)\n", code, *nextCode, code_size);
                write_bits(output, code, code_size);    
            }

            if (*nextCode < MAX_DICT_SIZE) {
                insert_to_dict(hashTablePtr, bucketCountsPtr, tableSize, next_str, *nextCode, entryCount);
                (*nextCode)++;
                if (*nextCode == 512) code_size = 10;
                else if (*nextCode == 1024) code_size = 11;
                else if (*nextCode == 2048) code_size = 12;
            } else {
                if (strlen(current_str) > 0) {
                    int code = get_code(current_str, hashTable, *tableSize);
                    write_bits(output, code, code_size);
                }
                flush_bits(output);
                write_bits(output, RESET_MARKER, code_size);

                for (int i = 0; i < *tableSize; i++) {
                    node* current = hashTable[i];
                    while (current) {
                        node* temp = current;
                        current = current->next;
                        free(temp);
                    }
                    hashTable[i] = NULL;
                    bucketCounts[i] = 0;
                }
                *entryCount = 0;
                dict_init(hashTablePtr, bucketCountsPtr, tableSize, nextCode, entryCount);
                code_size = 9;
                hashTable = *hashTablePtr;
                bucketCounts = *bucketCountsPtr;
            }

            current_str[0] = (char)c;
            current_str[1] = '\0';
        }
    }

    if (strlen(current_str) > 0) {
        int code = get_code(current_str, hashTable, *tableSize);
        write_bits(output, code, code_size);
    }

printf("Writing END_MARKER: %d (nextCode: %d, codeSize: %d)\n", END_MARKER, *nextCode, code_size);
flush_bits(output);
write_bits(output, END_MARKER, code_size);
flush_bits(output);

}

uint32_t bit_buffer = 0;
int bit_count = 0;

void write_bits(FILE *out, uint16_t code, int code_size) {
    bit_buffer = (bit_buffer << code_size) | code;
    bit_count += code_size;
    while (bit_count >= 8) {
        uint8_t byte = (bit_buffer >> (bit_count - 8)) & 0xFF;
        fputc(byte, out);
        bit_count -= 8;
    }
}

void flush_bits(FILE *out) {
    if (bit_count > 0) {
        uint8_t byte = (bit_buffer << (8 - bit_count)) & 0xFF;
        fputc(byte, out);
    }
    bit_buffer = 0;
    bit_count = 0;
}

int main() {
    char input_filename[256], output_filename[256];

    printf("Enter file name to read: ");
    scanf("%255s", input_filename);
    FILE* input = fopen(input_filename, "rb");
    if (!input) {
        perror("Input file error");
        return 1;
    }

    printf("Enter file name to export to: ");
    scanf("%255s", output_filename);
    FILE* output = fopen(output_filename, "wb");
    if (!output) {
        perror("Output file error");
        fclose(input);
        return 1;
    }

    int tableSize = INITIAL_TABLE_SIZE;
    node** hashTable = calloc(tableSize, sizeof(node*));
    int* bucketCounts = calloc(tableSize, sizeof(int));
    int entryCount = 0;
    int nextCode;

    dict_init(&hashTable, &bucketCounts, &tableSize, &nextCode, &entryCount);
    lzw_algo(input, &hashTable, &bucketCounts, &tableSize, &nextCode, output, &entryCount);

    fclose(input);
    fclose(output);

    for (int i = 0; i < tableSize; i++) {
        node* current = hashTable[i];
        while (current) {
            node* temp = current;
            current = current->next;
            free(temp);
        }
    }

    free(hashTable);
    free(bucketCounts);

    return 0;
}
