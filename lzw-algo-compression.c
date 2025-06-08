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

void resize_hash_table(node*** hashTablePtr, int** bucketCountsPtr, int* tableSize) {
    int oldSize = *tableSize;
    int newSize = oldSize * 2;

    node** oldTable = *hashTablePtr;
    int* oldCounts = *bucketCountsPtr;

    node** newTable = calloc(newSize, sizeof(node*));
    int* newCounts = calloc(newSize, sizeof(int));

    for (int i = 0; i < oldSize; i++) {
        node* current = oldTable[i];
        while (current) {
            node* next = current->next;

            unsigned long newHash = current->hashValue % newSize;
            current->next = newTable[newHash];
            newTable[newHash] = current;
            newCounts[newHash]++;

            current = next;
        }
    }

    free(oldTable);
    free(oldCounts);

    *hashTablePtr = newTable;
    *bucketCountsPtr = newCounts;
    *tableSize = newSize;

    printf("Resized hash table to size %d\n", newSize);
}

void insert_to_dict(node*** hashTablePtr, int** bucketCountsPtr, int* tableSize,
                    const char* word, int code, int* entryCount) {
    if ((double)(*entryCount) / *tableSize > LOAD_FACTOR_THRESHOLD) {
        resize_hash_table(hashTablePtr, bucketCountsPtr, tableSize);
    }

    node** hashTable = *hashTablePtr;
    int* bucketCounts = *bucketCountsPtr;

    node* new_node = create_node(word, code);
    int table_index = new_node->hashValue % *tableSize;

    new_node->next = hashTable[table_index];
    hashTable[table_index] = new_node;
    bucketCounts[table_index]++;
    (*entryCount)++;
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



void lzw_algo(FILE* fileptr, node** hashTable, int* bucketCounts, int* tableSize,
              int* nextCode, FILE* newFileptr, int* entryCount) {
    int c;
    char current_str[WORD_LEN] = "";
    char next_str[WORD_LEN];

    while ((c = fgetc(fileptr)) != EOF) {
        char byte[2] = {(char)c, '\0'};

        strcpy(next_str, current_str);
        strncat(next_str, byte, WORD_LEN - strlen(next_str) - 1);

        if (lookup(next_str, hashTable, *tableSize)) {
            strcpy(current_str, next_str);
        } else {
            if (strlen(current_str) > 0) {
                int code = get_code(current_str, hashTable, *tableSize);
                fprintf(newFileptr, "%d ", code);
            }

            insert_to_dict(&hashTable, &bucketCounts, tableSize, next_str, *nextCode, entryCount);
            (*nextCode)++;

            strcpy(current_str, byte);
        }
    }

    if (strlen(current_str) > 0) {
        int code = get_code(current_str, hashTable, *tableSize);
        fprintf(newFileptr, "%d ", code);
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





// i have no clue how to change the compression, i have been doing research and will resume after work and concert.








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

    int tableSize = INITIAL_TABLE_SIZE; 
    node** hashTable = calloc(tableSize, sizeof(node*));
    int* bucketCounts = calloc(tableSize, sizeof(int));
    int entryCount = 0;

    int nextCode;

    dict_init(&hashTable, &bucketCounts, &tableSize, &nextCode, &entryCount);
    lzw_algo(fileptr, hashTable, bucketCounts, &tableSize, &nextCode, newFileptr, &entryCount);

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





