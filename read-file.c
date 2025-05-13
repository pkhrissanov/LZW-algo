#include <stdio.h>

int main(){

    FILE *fileptr;
    fileptr = fopen("i-am-sick.txt", "r");
    
    char string[200];

    fgets(string, 200, fileptr);
    printf("%s", string);
    fclose(fileptr);

}