#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define TABLESIZE 768
#define WORDLEN 8
#define MAX_COLLISIONS 10000

typedef struct node {
    char word[WORDLEN];
    int code;
    int hashedVal;
} node;

typedef struct {
    char s[WORDLEN];
    int index;
} CollisionLog;

unsigned long hash(unsigned char *str) {
    unsigned long h = 5381;
    int c;
    while ((c = *str++)) {
        h = ((h << 5) + h) + c;
    }
    return h;
}

void dictReset(node *dict, bool *used) {
    for (int i = 0; i < TABLESIZE; i++) {
        memset(dict[i].word, 0, WORDLEN);
        dict[i].code = 0;
        dict[i].hashedVal = 0;
        used[i] = false;
    }
}

unsigned char *fileInput(long *outSize) {
    FILE *in = fopen("in", "rb");
    if (!in) {
        perror("Failed to open file");
        return NULL;
    }

    fseek(in, 0, SEEK_END);
    long size = ftell(in);
    rewind(in);

    if (size < 0) {
        perror("ftell failed");
        fclose(in);
        return NULL;
    }

    unsigned char *buffer = malloc(size);
    if (!buffer) {
        perror("malloc failed");
        fclose(in);
        return NULL;
    }

    size_t bytesRead = fread(buffer, 1, size, in);
    if (bytesRead != size) {
        perror("fread failed");
        free(buffer);
        fclose(in);
        return NULL;
    }

    fclose(in);
    *outSize = size;
    return buffer;
}

void collisionTest(unsigned char *data, long size) {
    node dict[TABLESIZE] = {0};
    bool used[TABLESIZE] = {0};

    CollisionLog collisionLog[MAX_COLLISIONS];
    int collisionCount = 0;

    int collisions = 0;
    int inserted = 0;
    int resets = 0;

    for (long i = 0; i < size - 2; i++) {
        char s[4] = { data[i], data[i+1], data[i+2], '\0' };
        unsigned long hashed = hash((unsigned char *)s);
        int index = hashed % TABLESIZE;

        printf("String: '%s' | Hash: %lu | Index: %d\n", s, hashed, index);

        if (!used[index]) {
            strncpy(dict[index].word, s, WORDLEN - 1);
            dict[index].word[WORDLEN - 1] = '\0';
            dict[index].code = inserted++;
            dict[index].hashedVal = hashed;
            used[index] = true;
        } else {
            if (strcmp(dict[index].word, s) != 0) {
                if (collisionCount < MAX_COLLISIONS) {
                    strncpy(collisionLog[collisionCount].s, s, WORDLEN - 1);
                    collisionLog[collisionCount].s[WORDLEN - 1] = '\0';
                    collisionLog[collisionCount].index = index;
                    collisionCount++;
                }
                collisions++;
                resets++;
                dictReset(dict, used);
                inserted = 0;
            }
        }
    }

    printf("\nTest complete:\n");
    printf("Total bytes processed: %ld\n", size);
    printf("Total resets:          %d\n", resets);
    printf("Total collisions:      %d\n", collisions);

    // Print grouped collisions
    printf("\n--- Collided Strings by Index ---\n");
    for (int i = 0; i < TABLESIZE; i++) {
        bool printed_header = false;
        for (int j = 0; j < collisionCount; j++) {
            if (collisionLog[j].index == i) {
                if (!printed_header) {
                    printf("Index %d:\n", i);
                    printed_header = true;
                }
                printf("  '%s'\n", collisionLog[j].s);
            }
        }
    }
}

int main() {
    long size;
    unsigned char *data = fileInput(&size);
    if (!data) return 1;

    collisionTest(data, size);

    free(data);
    return 0;
}
