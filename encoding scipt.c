#include <stdio.h>
#include <stdlib.h>

void initial_dict();

int main(){

    initial_dict();


}



void initial_dict() {
    for(int i = 0; i <= 127; ++i) {
        char *asci_char = malloc(2 * sizeof(char));
        asci_char[0] = (char)i;
        asci_char[1] = '\0';
        printf("the character %s has the ascii value %d\n", asci_char, i);
    }
}

/* to do 

write initial dictionary function
figure out how i am storing everything 
write store key and value function
write input stream for hash map 
hash map function for new strings? 



*/