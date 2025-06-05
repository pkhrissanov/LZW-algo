#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DICT_SIZE 4096

char* dictionary[MAX_DICT_SIZE];

void init_dictionary() {
    for (int i = 0; i < 256; i++) {
        dictionary[i] = malloc(2);
        dictionary[i][0] = (char)i;
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

void output_string(FILE *out, const char *str) {
    fputs(str, out);
}




int main(){

    char filename[256];
    debug();
    printf("Enter file name to decompres: ");
    scanf("%255s", filename);

    FILE* fileptr = fopen(filename, "rb");
    if (!fileptr) {
        printf("File name invalid.\n");
        return 1;
    } else {
        printf("File properly opened.\n");
    }



int next_code = ASCII_TABLE_SIZE; 
int previous_code, current_code;
char *previous_string, *current_string;


previous_string = dictionary[previous_code];
output_string(output, previous_string);

// Main loop
while (read_next_code(&current_code, input)) { 
    if (current_code < next_code && dictionary[current_code]) {
        current_string = dictionary[current_code];
    } else {
        size_t len = strlen(previous_string);
        current_string = malloc(len + 2);
        strcpy(current_string, previous_string);
        current_string[len] = previous_string[0];
        current_string[len + 1] = '\0';
    }

    output_string(output, current_string);

    if (next_code < MAX_DICT_SIZE) {
        size_t len_prev = strlen(previous_string);
        char *new_entry = malloc(len_prev + 2);
        strcpy(new_entry, previous_string);
        new_entry[len_prev] = current_string[0];
        new_entry[len_prev + 1] = '\0';
        dictionary[next_code++] = new_entry;
    }

    previous_string = current_string;
    previous_code = current_code;
}


    













}




//need to make the actual decomp algo 
//theoretically, i can just have a array that keeps all the codes, for any code that is larger than the 256 it gives appneded to the end.
//have the dictionary code be the position in the array itself