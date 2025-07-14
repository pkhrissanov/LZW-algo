#include <stdio.h>
int main(){

FILE *f = fopen("incomp", "rb");
int c;
while ((c = fgetc(f)) != EOF) {

    if(c == 4096) {
	printf("BIT BUMP");}

    printf("%04d  ", c);
 }
fclose(f);
}
