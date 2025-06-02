#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define TABLE_SIZE 541
#define WORD_LEN 100
#define DEBUG false

// Debug macro
#define DEBUG_PRINT(fmt, ...) \
    do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

unsigned long hash(unsigned char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
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

void insert_to_dict(node* hashTable[], int* bucketCounts, const char* word, int code) {
    node* new_node = create_node(word, code);
    int table_index = new_node->hashValue % TABLE_SIZE;
    new_node->next = hashTable[table_index];
    hashTable[table_index] = new_node;
    bucketCounts[table_index]++;
}

bool lookup(const char* key, node* hashTable[]) {
    unsigned long h = hash((unsigned char*)key);
    int index = h % TABLE_SIZE;
    node* current = hashTable[index];

    while (current) {
        if (strcmp(current->word, key) == 0) {
            return true;
        }
        current = current->next;
    }
    return false;
}

int get_code(const char* key, node* hashTable[]) {
    unsigned long h = hash((unsigned char*)key);
    int index = h % TABLE_SIZE;
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

void dict_init(node* hashTable[], int* bucketCounts, int* nextCode) {
    for (int i = 0; i < 256; i++) {
        char str[2] = {(char)i, '\0'};
        insert_to_dict(hashTable, bucketCounts, str, i);
    }
    *nextCode = 256;
}

void lzw_algo(FILE* fileptr, node* hashTable[], int* bucketCounts, int* nextCode, FILE* newFileptr) {
    int c;
    char current_str[WORD_LEN] = "";
    char next_str[WORD_LEN];

    while ((c = fgetc(fileptr)) != EOF) {
        char byte[2] = {(char)c, '\0'};

        strcpy(next_str, current_str);
        strncat(next_str, byte, WORD_LEN - strlen(next_str) - 1);

        if (lookup(next_str, hashTable)) {
            strcpy(current_str, next_str);
        }else {
        if (strlen(current_str) > 0) {
        int code = get_code(current_str, hashTable);
        fprintf(newFileptr, "%d ", code);
        DEBUG_PRINT("Output code: %d for '%s'\n", code, current_str);
    }

        insert_to_dict(hashTable, bucketCounts, next_str, *nextCode);
        DEBUG_PRINT("Inserted '%s' with code %d\n", next_str, *nextCode);
        (*nextCode)++;


        strcpy(current_str, byte);  
    }

    if (strlen(current_str) > 0) {
        int code = get_code(current_str, hashTable);
        fprintf(newFileptr, "%d ", code);
        DEBUG_PRINT("Final output code: %d for '%s'\n", code, current_str);
    }
}

void print_bucket_counts(int* bucketCounts) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        if (bucketCounts[i] > 0) {
            printf("Bucket %d: %d item(s)\n", i, bucketCounts[i]);
        }
    }
}

/*if (strlen(current_str) > 0) {
    int code = get_code(current_str, hashTable);
    fprintf(newFileptr, "%d", code);
}*/














int main() {
    char input_filename[256], output_filename[256];

    printf("Enter file name to read: ");
    scanf("%255s", input_filename);
    FILE* fileptr = fopen(input_filename, "rb");
    if (!fileptr) {
        printf("File name invalid.\n");
        return 1;
    }

    printf("Enter file name to export to: ");
    scanf("%255s", output_filename);
    FILE* newFileptr = fopen(output_filename, "w");
    if (!newFileptr) {
        printf("Output file name invalid.\n");
        fclose(fileptr);
        return 1;
    }

    node* hashTable[TABLE_SIZE] = {NULL};
    int bucketCounts[TABLE_SIZE] = {0};
    int nextCode;

    dict_init(hashTable, bucketCounts, &nextCode);
    lzw_algo(fileptr, hashTable, bucketCounts, &nextCode, newFileptr);

    fclose(fileptr);
    fclose(newFileptr);

    print_bucket_counts(bucketCounts);

    for (int i = 0; i < TABLE_SIZE; i++) {
        node* current = hashTable[i];
        while (current) {
            node* temp = current;
            current = current->next;
            free(temp);
        }
    }

    return 0;
}



    //test to see where the initial characters are gettitng printed too 

 /*   unsigned long h = hash((unsigned char*)ascii_str);
        debug();
    int index = h % TABLE_SIZE;
        debug();
    node* current = hashTable[index];
        debug();

    while (current) {
        if (strcmp(current->word, ascii_str) == 0) {
            printf("ASCII '%c' (%d) => code %d (bucket %d)\n",
                (i >= 32 && i < 127) ? i : '.', i, current->index, index);
                    debug();
            break;
        }
        current = current->next;
    }

    if (!current) {
        printf("Missing ASCII code %d in dictionary.\n", i);
            debug();
    } */





