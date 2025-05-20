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


unsigned int hash(char* name){
    int hash_value = 0;
    int nameLength = strlen(name);
    for(int i=0; i < nameLength; i++){
        hash_value += name[i];
    }
    
    return hash_value;  
}



int main(){
    printf("Alexis -> %u\n", hash("Alexis"));
    printf("Peter -> %u\n", hash("Peter"));
    printf("Ron -> %u\n", hash("Ron"));
    printf("Mike -> %u\n", hash("Mike"));

    return 0;


}
