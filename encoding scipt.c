#include <stdio.h>
#include <stdlib.h>

void initial_dict();
void read_file();

int main(){

    read_file();


}



void initial_dict() {
    for(int i = 0; i <= 127; ++i) {
        char *asci_char = malloc(2 * sizeof(char));
        asci_char[0] = (char)i;
        asci_char[1] = '\0';
        printf("the character %s has the ascii value %d\n", asci_char, i);
    }
}

void read_file()
{
    char buffer[100];
    printf("Enter file that you want to compress: ");
    scanf("%s", &buffer);
    FILE *fileName = fopen(buffer, "r");

    while(!feof(fileName)) {
        char charEval = getc(fileName);
        char charAsci = (char)charEval;
        printf("%c = %d\n", charEval, charAsci);
    }
}



/* to do 

write initial dictionary function
figure out how i am storing everything 
write store key and value function
write input stream for hash map 
hash map function for new strings? 



*/