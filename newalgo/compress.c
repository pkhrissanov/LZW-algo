#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

//hello world


/*
    To Do List:

    ✅ figure out what can be reused

    🔧 make reset function [x]
    🔧 write logic for reset [ ]
    🔧 write file input pipeline [x]
    🔧 make grab char (from stream) function [ ]
    🔧 make write node function [x]
    🔧 make write output function [ ]
    🔧 make lookup function [ ]
    make hashing function [x]

    Optional:
    - 🔍 add debug print for dictionary contents
    - 🧪 test full compression pipeline with sample input
*/


#define TABLESIZE 1024
#define WORDLEN 8

typedef struct node {
    char word[WORDLEN];  
    int code;            
    int hashedVal; 
} node;

unsigned long hash(unsigned char *str) {
    unsigned long h = 5381;
    int c;
    while ((c = *str++)) {
        h = ((h << 5) + h) + c;
    }
    return h;
}


void ascii_init(node *dict) {
    for (int i = 0; i <= 255; i++) {
        char s[2] = { (char)i, '\0' }; 
        strncpy(dict[i].word, s, WORDLEN);
        dict[i].code = i;
        dict[i].hashedVal = hash((unsigned char *)s);
    }
}

//can be used for reset too imo 
void char_init(node *dict){
    for (int i = 256; i <= TABLESIZE; i++) {
        char s[2] = { (char)i, '\0' };
        strncpy(dict[i].word, s, WORDLEN - 1);
        dict[i].word[WORDLEN - 1] = '\0';
        dict[i].code = i;
        dict[i].hashedVal = hash((unsigned char *)s);
    }
}


bool collision(char *string, node *dict){
    int hashedString = hash(string);
    int index = hashedString % TABLESIZE;

    if (strcmp(dict[index].word, string) != 0){
        return true;
    }
    else if (dict[index].code == 0){
    return false;
    }
    else {
        return false;
    }
}


unsigned char *fileInput(){ 
    FILE *in = fopen("in", "rb");

    fseek(in, 0, SEEK_END);
    long size = ftell(in);
    rewind(in);

    if (size < 0) {
        perror("ftell failed");
        fclose(in);
        return NULL;
    }

    unsigned char *buffer = malloc(size);
    if (buffer == NULL) {
        perror("malloc failed");
        fclose(in);
        return NULL;
    }


    size_t bytesRead = fread(buffer, 1, size, in);
    if (bytesRead != size) {
        perror("fread failed");
        free(buffer);
        fclose(in);
        return NULL;
    }

    fclose(in);
    return buffer;
}



void insert(char *string, node *dict, int code, int hashedString){
    int index = hashedString % TABLESIZE;

    strncpy(dict[index].word, string, WORDLEN-1);
    dict[index].code = code;
    dict[index].hashedVal = hashedString;

}



bool lookup(char *string, node *dict){
    int hashedString = hash(string);
    int index = hashedString % TABLESIZE;

    if (dict[index].word == 0){
        return true;
    }




}

//void write(buffer, int code, FILE *outcomp){

//to do


//}







int main() {
    node dict[TABLESIZE];
    ascii_init(dict);

    for (int i = 0; i <= 255; i++) {
        printf("Index %3d | Word: '%s' | Code: %3d | Hash: %lu\n", i, dict[i].word, dict[i].code, (unsigned long)dict[i].hashedVal);
    }

    printf("test");

    char letter[] = {'A'};
    printf("%p", letter);
    if(collision(letter, dict)){
        printf("theres a crash");
    }
    else{
        printf("there is no crash");
    }
    return 0;
}