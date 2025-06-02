#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DICT_SIZE 4096

char* dictionary[MAX_DICT_SIZE];

void init_dictionary() {
    for (int i = 0; i < 256; i++) {
        dictionary[i] = malloc(2);
        dictionary[i][0] = i;
        dictionary[i][1] = '\0';
    }
}


void free_dictionary(int dictSize) {
    for (int i = 0; i < dictSize; i++) {
        if (dictionary[i]) {
            free(dictionary[i]);
        }
    }
}



//need to make the actual decomp algo 
//theoretically, i can just have a array that keeps all the codes, for any code that is larger than the 256 it gives appneded to the end.
//have the dictionary code be the position in the array itself