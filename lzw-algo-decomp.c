#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#define MAX_DICT_SIZE 4096
#define ASCII_TABLE_SIZE 256

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




int read_next_code(FILE *fp, int *code) {
    static int buffer = 0;
    static int bits_in_buffer = 0;

    while (bits_in_buffer < 12) {
        int next_byte = fgetc(fp);
        if (next_byte == EOF) {
            if (bits_in_buffer == 0) return 0; 
            else {
                
                buffer <<= (12 - bits_in_buffer);
                *code = buffer & 0xFFF;
                bits_in_buffer = 0;
                buffer = 0;
                return 1;
            }
        }
        buffer = (buffer << 8) | next_byte;
        bits_in_buffer += 8;
    }
  bits_in_buffer -= 12;
    *code = (buffer >> bits_in_buffer) & 0xFFF;

    return 1;
}




int main(){

init_dictionary();

char filename[256];
char outputname[256];

printf("Enter file name to decompress: ");
scanf("%255s", filename);

FILE* input = fopen(filename, "rb");
if (!input) {
    printf("File name invalid.\n");
    return 1;
}

printf("Enter output file name: ");
scanf("%255s", outputname);

FILE* output = fopen(outputname, "w");
if (!output) {
    printf("File name invalid.\n");
    fclose(input);
    return 1;
}

int next_code = ASCII_TABLE_SIZE; 
int previous_code, current_code;
char *previous_string, *current_string;

if (!read_next_code(input, &previous_code)) {
    printf("Empty or corrupt file.\n");
    fclose(input);
    fclose(output);
    return 1;
}

previous_string = dictionary[previous_code];
output_string(output, previous_string);

while (read_next_code(input, &current_code)) {
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

    if (current_code >= next_code) {
        free(current_string); 
    }

    previous_string = dictionary[current_code];  
    previous_code = current_code;
}

free_dictionary(next_code);
fclose(input);
fclose(output);
return 0;
}




//need to make the actual decomp algo 
//theoretically, i can just have a array that keeps all the codes, for any code that is larger than the 256 it gives appneded to the end.
//have the dictionary code be the position in the array itself