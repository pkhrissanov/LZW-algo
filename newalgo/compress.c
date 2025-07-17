#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>




/*
    To Do List:

    ✅ figure out what can be reused

    🔧 make reset function [ ]
    🔧 write logic for reset [ ]
    🔧 write file input pipeline [ ]
    🔧 make grab char (from stream) function [ ]
    🔧 make write node function [ ]
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
    int hashVal; 
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
        dict[i].hashVal = hash((unsigned char *)s);
    }
}

void char_init(node *dict){
    for (int i = 256; i <= 1024; i++) {
        char s[2] = { (char)i, '\0' };
        strncpy(dict[i].word, s, WORDLEN - 1);
        dict[i].word[WORDLEN - 1] = '\0';
        dict[i].code = i;
        dict[i].hashVal = hash((unsigned char *)s);
    }
}


bool collision(int hashVal, node *dict){
    if (dict[hashVal].hashVal == hashVal && dict[hashVal].code != 0){
        return true;
    }
    else {
        return false;
    }
}



int main() {
    node dict[TABLESIZE];
    ascii_init(dict);

    for (int i = 0; i < 255; i++) {
        printf("Index %3d | Word: '%s' | Code: %3d | Hash: %lu\n",
               i, dict[i].word, dict[i].code, (unsigned long)dict[i].hashVal);
    }
    if(collision(65, dict)){
        printf("theres a crash");
    }
    return 0;
}
