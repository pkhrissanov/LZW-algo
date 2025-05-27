#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define TABLE_SIZE 541
#define WORD_LEN 100


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



void dict_init(node* hashTable[], int* bucketCounts, int* nextCode) {
    for (int i = 0; i < 256; i++) {
        char str[2] = {(char)i, '\0'};
        insert_to_dict(hashTable, bucketCounts, str, i);
    }
    *nextCode = 256;
}


void print_bucket_counts(int *bucketCounts) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        printf("Bucket %d: %d item(s)\n", i, bucketCounts[i]);
    }
    
}



int main() {
    char filename[256];
    printf("Enter file name to read: ");
    scanf("%255s", filename);

    FILE* fileptr = fopen(filename, "r");
    if (!fileptr) {
        printf("File name invalid.\n");
        return 1;
    } else {
        printf("File properly opened.\n");
    }


    node* hashTable[TABLE_SIZE] = {NULL};
    int bucketCounts[TABLE_SIZE] = {0};
    int nextCode;

    dict_init(hashTable, bucketCounts, &nextCode);

    //lzw_algo(fileptr, hashTable, bucketCounts, nextCode);

    fclose(fileptr);

    print_bucket_counts(bucketCounts);
    for (int i = 0; i < 256; i++) {
    char ascii_str[2] = { (char)i, '\0' };


    //test to see where the initial characters are gettitng printed too 

    unsigned long h = hash((unsigned char*)ascii_str);
    int index = h % TABLE_SIZE;
    node* current = hashTable[index];

    while (current) {
        if (strcmp(current->word, ascii_str) == 0) {
            printf("ASCII '%c' (%d) => code %d (bucket %d)\n",
                (i >= 32 && i < 127) ? i : '.', i, current->index, index);
            break;
        }
        current = current->next;
    }

    if (!current) {
        printf("Missing ASCII code %d in dictionary.\n", i);
    }
}





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
