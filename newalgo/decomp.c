#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define TABLESIZE 1024
#define WORDLEN 32
#define FIRSTCODE 256
#define CODE_BIT_WIDTH 10
#define CODE_MAX ((1 << CODE_BIT_WIDTH) - 1)


uint32_t bitbuffer = 0;
int bitcount = 0;

typedef struct node {
    char word[WORDLEN];
    int code;
} node;


void ascii_init(node *dict) {
    for (int i = 0; i <= 255; i++) {
        char s[2] = { (char)i, '\0' };
        strncpy(dict[i].word, s, WORDLEN - 1);
        dict[i].word[WORDLEN - 1] = '\0';
        dict[i].code = i;
    }
    for (int i = 256; i < TABLESIZE; i++) {
        dict[i].word[0] = '\0';
        dict[i].code = -1;
    }
}

void dictReset(node *dict) {
    for (int i = 256; i < TABLESIZE; i++) {
        dict[i].word[0] = '\0';
        dict[i].code = -1;
    }
}


int readCode(FILE *in) {
    while (bitcount <= 22) {
        int byte = fgetc(in);
        if (byte == EOF) break;
        bitbuffer |= (uint32_t)byte << (24 - bitcount);
        bitcount += 8;
    }
    if (bitcount < CODE_BIT_WIDTH) return -1;

    int code = bitbuffer >> (32 - CODE_BIT_WIDTH);
    bitbuffer <<= CODE_BIT_WIDTH;
    bitcount -= CODE_BIT_WIDTH;
    return code;
}

void lzw_decompress(FILE *in, FILE *out) {
    node dict[TABLESIZE];
    ascii_init(dict);

    int nextcode = FIRSTCODE;
    int oldcode = readCode(in);
    if (oldcode == -1) return;

    char current[WORDLEN];
    strncpy(current, dict[oldcode].word, WORDLEN - 1);
    current[WORDLEN - 1] = '\0';
    fputs(current, out);
    printf("DEC: code=%d string='%s'\n", oldcode, current);

    int newcode;
    while ((newcode = readCode(in)) != -1) {
        char entry[WORDLEN] = {0};

        if (dict[newcode].code != -1) {
            strncpy(entry, dict[newcode].word, WORDLEN - 1);
            entry[WORDLEN - 1] = '\0';
        } else {
            size_t len = strlen(current);
            strncpy(entry, current, WORDLEN - 1);
            if (len < WORDLEN - 1) {
                entry[len] = current[0];
                entry[len + 1] = '\0';
            } else {
                entry[WORDLEN - 1] = '\0';
            }
        }

        fputs(entry, out);
        printf("DEC: code=%d string='%s'\n", newcode, entry);

        char newword[WORDLEN];
        size_t len2 = strlen(current);
        strncpy(newword, current, WORDLEN - 1);
        if (len2 < WORDLEN - 1) {
            newword[len2] = entry[0];
            newword[len2 + 1] = '\0';
        } else {
            newword[WORDLEN - 1] = '\0';
        }

        if (nextcode < TABLESIZE) {
            strncpy(dict[nextcode].word, newword, WORDLEN - 1);
            dict[nextcode].word[WORDLEN - 1] = '\0';
            dict[nextcode].code = nextcode;
            printf("DICT ADD: code=%d word='%s'\n", nextcode, newword);
            nextcode++;
        } else {
            printf("DICT RESET\n");
            dictReset(dict);
            ascii_init(dict);
            nextcode = FIRSTCODE;
        }

        strncpy(current, entry, WORDLEN - 1);
        current[WORDLEN - 1] = '\0';
    }
}

void check_result(const char *origfile, const char *decompfile) {
    FILE *f1 = fopen(origfile, "rb");
    FILE *f2 = fopen(decompfile, "rb");
    if (!f1 || !f2) {
        perror("fopen check");
        if (f1) fclose(f1);
        if (f2) fclose(f2);
        return;
    }

    int c1, c2, pos = 0;
    while (1) {
        c1 = fgetc(f1);
        c2 = fgetc(f2);
        if (c1 == EOF && c2 == EOF) {
            printf("✅ Decompression MATCH: files are identical\n");
            break;
        }
        if (c1 != c2) {
            printf("❌ MISMATCH at offset %d: orig='%c' decomp='%c'\n",
                   pos, (c1 == EOF ? '?' : c1), (c2 == EOF ? '?' : c2));
            break;
        }
        pos++;
    }

    fclose(f1);
    fclose(f2);
}

int main() {
    FILE *in = fopen("outcomp", "rb");
    if (!in) {
        perror("fopen input");
        return 1;
    }
    FILE *out = fopen("outdecomp", "wb");
    if (!out) {
        perror("fopen output");
        fclose(in);
        return 1;
    }
    lzw_decompress(in, out);
    fclose(in);
    fclose(out);


    check_result("in", "outdecomp");
    return 0;
}
