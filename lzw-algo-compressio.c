#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define TABLE_SIZE 541
#define WORD_LEN 100
#define DEBUG false

void debug(){
    if(DEBUG){
        printf("debugging\n");
    }
}

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
    for (int i = 0; i <256; i++) {
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


void lzw_algo(FILE* fileptr, node* hashTable[], int* bucketCounts, int * nextCode){
    int c;
    char current_str[WORD_LEN] = "";
    char next_str[WORD_LEN];
    
    while ((c = fgetc(fileptr)) != EOF) {
        char byte[2] = {(char)c, '\0'};
    
    strcpy(next_str, current_str);
    strncat(next_str, byte, WORD_LEN - strlen(next_str) - 1);
    
    if (lookup(next_str, hashTable)) {
        strcpy(current_str, next_str);
    } else {
        int code = get_code(current_str, hashTable);
        printf("%d ", code);
        insert_to_dict(hashTable, bucketCounts, next_str, *nextCode);
        (*nextCode)++;
        strcpy(current_str, byte);
    }
}
if (strlen(current_str) > 0) {
    int code = get_code(current_str, hashTable);
    printf("%d ", code);
}
}








int main() {
    char filename[256];
    debug();
    printf("Enter file name to read: ");
    scanf("%255s", filename);

    FILE* fileptr = fopen(filename, "rb");
    if (!fileptr) {
        printf("File name invalid.\n");
        return 1;
    } else {
        printf("File properly opened.\n");
    }
    debug();
    node* hashTable[TABLE_SIZE] = {NULL};
    for (int i = 0; i < TABLE_SIZE; ++i)
    hashTable[i] = NULL;
    debug();
    int bucketCounts[TABLE_SIZE] = {0};
    debug();
    int nextCode;
    debug();

    dict_init(hashTable, bucketCounts, &nextCode);
        debug();

    lzw_algo(fileptr, hashTable, bucketCounts, &nextCode);
        debug();




    fclose(fileptr);
        debug();

    print_bucket_counts(bucketCounts);
    for (int i = 0; i < 256; i++) {
    char ascii_str[2] = { (char)i, '\0' };
        debug();


    //test to see where the initial characters are gettitng printed too 

    unsigned long h = hash((unsigned char*)ascii_str);
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
        debug();

    return 0;
}
