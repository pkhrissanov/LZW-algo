#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_NAME 256
#define TABLE_SIZE 10


typedef struct{
    char name[MAX_NAME]; 
    int age;

} person;


unsigned int hash(char* word){
    int hash_value = 0;
    int nameLength = strlen(word);
    for(int i=0; i < nameLength; i++){
        hash_value += word[i];
    }
    
    return hash_value;  
}



int main(){
    printf("Alexis -> %u\n", hash("Alexis"));
    printf("Peter -> %u\n", hash("Peter"));
    printf("Ron -> %u\n", hash("Ron"));
    printf("Mike -> %u\n", hash("Mike"));



    printf("Enter File name to read: ");
    char buffer[1024];
    scanf("%s", &buffer);
    FILE *fileptr;
    fileptr = fopen(buffer, "r");

    if(fileptr = NULL){
        printf("file name invalid");
    }
    else{
        printf("file properly opened");
    }

    char word[100];
    while (fscanf(fileptr, "%99s", word) == 1) {
    printf("Starting to read...\n");
    printf("Read word: %s\n", word);
    }






    return 0;


}


/*
logic for program

allocate rather small array, want to be able to 
hash function for each word




*/