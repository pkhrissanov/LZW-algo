#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void initial_dict();
void read_file();
HashNode* create_node(char* key, int code);
void insert(HashNode** table, char* key, int code);


typedef struct HashNode {
    char* key;
    int code;
    struct HashNode* next; 
} HashNode;




int main(){
    HashNode **dict = malloc(4096 * sizeof(HashNode*));
    for (int i = 0; i < 4096; i++) {
        dict[i] = NULL;
    }



    HashNode* new_node = malloc(sizeof(HashNode));
    printf("please enter the letter that you want to store in the first node: ");
    char *input = malloc(2);
    scanf("%s", input);

    new_node->key = strdup(input);
    new_node->code = (int)input[0];
    new_node->next = NULL;

    dict[0] = new_node;
    printf("Inserted key: %s with code: %d at index: %d with the adress %p \n", dict[0]->key, dict[0]->code, 0, &new_node);




    HashNode* second_node = malloc(sizeof(HashNode));
    new_node->next = second_node;
    printf("please enter the letter that you want to store in the second node: ");
    char *new_input = malloc(2);
    scanf("%s", new_input);

    new_node->key = strdup(new_input);
    new_node->code = (int)new_input[0];
    new_node->next = NULL;
    printf("Inserted key: %s with code: %d at index: %d with the adress %p \n", new_node->key, new_node->code, 0, &second_node);

}

void initial_dict() {
    for(int i = 0; i <= 127; ++i) {
        char *asci_char = malloc(2 * sizeof(char));
        asci_char[0] = (char)i;
        asci_char[1] = '\0';
        printf("the character %s has the ascii value %d\n", asci_char, i);
    }
}

void read_file()
{
    char buffer[100];
    printf("Enter file that you want to compress: ");
    scanf("%s", &buffer);
    FILE *fileName = fopen(buffer, "r");

    while(!feof(fileName)) {
        char charEval = getc(fileName);
        char charAsci = (char)charEval;
        printf("%c = %d\n", charEval, charAsci);
    }
}



HashNode* create_node(char* key, int code) {
    HashNode* node = malloc(sizeof(HashNode));
    node->key = strdup(key);
    node->code = code;
    node->next = NULL;
    return node;
}

void insert(HashNode** table, char* key, int code) {
    unsigned int index = hash(key) % TABLE_SIZE; /* not sure how this works yet, this was taken from internet. must learn how hash stuff actually works*/
    HashNode* new_node = create_node(key, code);
    new_node->next = table[index];
    table[index] = new_node;
}




/* 

    might not be as much code todya, reading up on hashing to see how to implement at write




    logic for lzw 
    create initial dict

    i=0
    buffer [1024];

    start reading 
        if table(hash(string)) != to anything in the bucket 
            insert
        if 



*/




/* to do 
have to figure out the hash table 
    will be making it for the initial dictionary aswell
    must create a array of linked lists - figure out how to create linked list struct for the array 
    figure out the actual allocation of array of linked lists 


what am i actually storing in each node? 
    the string that is encoded
    the value of the the string encoded
    the pointer to next string 




figure out how i am storing everything 
write store key and value function
write input stream for hash map 
hash map function for new strings? 



*/