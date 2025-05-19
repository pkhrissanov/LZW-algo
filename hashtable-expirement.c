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
    return 5;
}



int main(){
    printf("Jacob -> %u\n", hash("Jacob"));
    printf("Natalie -> %u\n", hash("Natalie"));
    printf("Alexis -> %u\n", hash("Alexis"));
    printf("Peter -> %u\n", hash("Peter"));
    return 0;


}
