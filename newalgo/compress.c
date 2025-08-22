#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define TABLESIZE 1024
#define WORDLEN 32
#define FIRSTCODE 256
#define CODE_BIT_WIDTH 10                 // << fixed 10-bit output
#define CODE_MAX ((1 << CODE_BIT_WIDTH) - 1)

// Bit writing globals
uint32_t bitbuffer = 0;
int bitcount = 0;

typedef struct node {
    char word[WORDLEN];
    int code;
    int hashedVal;
} node;

// Hash function
unsigned long hash(unsigned char *str) {
    unsigned long h = 5381;
    int c;
    while ((c = *str++)) {
        h = ((h << 5) + h) + c;
    }
    return h;
}

// Init ASCII into dictionary
void ascii_init(node *dict) {
    for (int i = 0; i <= 255; i++) {
        char s[2] = { (char)i, '\0' };
        strncpy(dict[i].word, s, WORDLEN);
        dict[i].code = i;
        dict[i].hashedVal = hash((unsigned char *)s);
    }
    for (int i = 256; i < TABLESIZE; i++) {
        dict[i].word[0] = '\0';
        dict[i].code = -1;
        dict[i].hashedVal = -1;
    }
}

void dictReset(node *dict) {
    for (int i = 256; i < TABLESIZE; i++) {
        dict[i].word[0] = '\0';
        dict[i].code = -1;
        dict[i].hashedVal = -1;
    }
}

bool collision(char *string, node *dict) {
    int hashedString = hash((unsigned char *)string);
    int index = hashedString % TABLESIZE;
    return strcmp(dict[index].word, string) != 0 && dict[index].code != -1;
}

unsigned char *fileInput() {
    FILE *in = fopen("in", "rb");
    if (!in) {
        perror("fopen");
        return NULL;
    }

    fseek(in, 0, SEEK_END);
    long size = ftell(in);
    rewind(in);
    if (size < 0) {
        perror("ftell");
        fclose(in);
        return NULL;
    }

    unsigned char *buffer = malloc(size + 1);
    if (!buffer) {
        perror("malloc");
        fclose(in);
        return NULL;
    }

    size_t bytesRead = fread(buffer, 1, size, in);
    if (bytesRead != (size_t)size) {
        perror("fread");
        free(buffer);
        fclose(in);
        return NULL;
    }

    buffer[size] = '\0';
    fclose(in);
    return buffer;
}

void insert(char *string, node *dict, int code, int hashedString) {
    int index = hashedString % TABLESIZE;
    while (dict[index].code != -1 && strcmp(dict[index].word, string) != 0) {
        index = (index + 1) % TABLESIZE;
    }
    strncpy(dict[index].word, string, WORDLEN - 1);
    dict[index].word[WORDLEN - 1] = '\0';
    dict[index].code = code;
    dict[index].hashedVal = hashedString;
}

int lookup(char *string, node *dict) {
    int hashedString = hash((unsigned char *)string);
    int index = hashedString % TABLESIZE;
    int start = index;

    if (strlen(string) == 1 && (unsigned char)string[0] < 256) {
        return (unsigned char)string[0];
    }

    while (dict[index].code != -1) {
        if (strcmp(dict[index].word, string) == 0)
            return dict[index].code;
        index = (index + 1) % TABLESIZE;
        if (index == start) break;
    }

    return -1;
}

char grabChar(unsigned char in[], int pos) {
    return in[pos];
}

void writeCode(FILE *out, int code) {
    if (code < 0 || code > CODE_MAX) {
        fprintf(stderr, "ERROR: code %d out of 10-bit range (0..%d)\n", code, CODE_MAX);
        exit(1);
    }
    bitbuffer |= ((uint32_t)code << (32 - CODE_BIT_WIDTH - bitcount));
    bitcount += CODE_BIT_WIDTH;
    while (bitcount >= 8) {
        uint8_t byte = bitbuffer >> 24;
        fwrite(&byte, 1, 1, out);
        bitbuffer <<= 8;
        bitcount -= 8;
    }
    printf("OUT: code=%d width=%d\n", code, CODE_BIT_WIDTH);
}

void flushBits(FILE *out) {
    while (bitcount > 0) {
        uint8_t byte = bitbuffer >> 24;
        fwrite(&byte, 1, 1, out);
        bitbuffer <<= 8;
        bitcount -= 8;
    }
}

void lzw_compress(unsigned char *in, FILE *out) {
    node dict[TABLESIZE];
    ascii_init(dict);
    for (int i = 0; i <= 255; i++) {
        char s[2] = { (char)i, '\0' };
        int code = lookup(s, dict);
        if (code != i) {
            printf("FAIL: lookup('%s') = %d, expected %d\n", s, code, i);
        }
    }
    int nextcode = FIRSTCODE;

    char current[WORDLEN] = {0};
    char temp[WORDLEN] = {0};
    current[0] = in[0];
    current[1] = '\0';

    for (int i = 1; in[i] != '\0'; i++) {
        char c = grabChar(in, i);

        strncpy(temp, current, WORDLEN - 2);
        temp[WORDLEN - 2] = '\0';
        size_t len = strlen(temp);
        if (len < WORDLEN - 1) {
            temp[len] = c;
            temp[len + 1] = '\0';
        }

        if (lookup(temp, dict) != -1) {
            strncpy(current, temp, WORDLEN);
        } else {
            int code = lookup(current, dict);
            if (code == -1) {
                fprintf(stderr, "ERROR: lookup failed for '%s'\n", current);
                exit(1);
            }
            writeCode(out, code); // always 10 bits
            if (nextcode < TABLESIZE) {
                insert(temp, dict, nextcode++, hash((unsigned char *)temp));
            } else {
                dictReset(dict);
                nextcode = FIRSTCODE;
            }
            current[0] = c;
            current[1] = '\0';
        }
    }

    if (current[0] != '\0') {
        int code = lookup(current, dict);
        if (code == -1) {
            fprintf(stderr, "ERROR: lookup failed for '%s'\n", current);
            exit(1);
        }
        writeCode(out, code); // always 10 bits
    }
    flushBits(out);
}

int main() {
    unsigned char *in = fileInput();
    if (!in) return 1;
    FILE *out = fopen("outcomp", "wb");
    if (!out) {
        perror("fopen output");
        free(in);
        return 1;
    }
    lzw_compress(in, out);
    fclose(out);
    free(in);
    return 0;
}
