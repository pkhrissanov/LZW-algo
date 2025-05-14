#include <stdio.h>

int main(){

    FILE *fileptr;
    fileptr = fopen("i-am-sick.txt", "r");
    char string;


    while (!feof(fileptr)){
       string =  getc(fileptr);
        printf("%c \n", string);
    }
    fclose(fileptr);

}