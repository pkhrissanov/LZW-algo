#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_NAME 256
#define TABLE_SIZE 10
#define WORD_LEN 100



unsigned long hash(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}


typedef struct node {
    char word[100];
    unsigned long hashValue;
    int index;
    struct node* next;
} node;




node* create_node(const char* word) {
    node* new_node = malloc(sizeof(node));
    if (new_node == NULL) {
        perror("Memory allocation failed");
        exit(1);
    }
    strncpy(new_node->word, word, strlen(word));
    new_node->word[strlen(word)] = '\0'; 
    new_node-> hashValue = hash(word);
    new_node-> index = new_node->hashValue % TABLE_SIZE;
    new_node->next = NULL;
    return new_node;
}





int main(){

    printf("Enter File name to read: ");
    char buffer[1024];
    scanf("%s", buffer);
    FILE *fileptr;
    fileptr = fopen(buffer, "r");

    if(fileptr == NULL){
        printf("file name invalid");
    }
    else{
        printf("file properly opened");
    }



    char word[WORD_LEN];

    if (fscanf(fileptr, "%99s", word) != 1) {
        printf("No words in file.\n");
        fclose(fileptr);
        return 1;
    }

    node* head = create_node(word);
    node* current = head;

    while (fscanf(fileptr, "%99s", word) == 1) {
        current->next = create_node(word);
        current = current->next;
    }

    fclose(fileptr);




    printf("Words in the list:\n");
    current = head;
    while (current) {
        printf("word: %s        ", current->word);
        printf("hashed value: %u        ", current->hashValue);
        printf("table index: %d\n", current->index);
        current = current->next;
    }



    current = head;
    while (current) {
        node* temp = current;
        current = current->next;
        free(temp);
    }

    return 0;
}



/* count number in each bucket
    while next != null
        count += 1 
        go to next node


*/
