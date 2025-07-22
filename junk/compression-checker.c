#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DICT_SIZE 4096
#define WORD_LEN 100

char* dict[MAX_DICT_SIZE];

void init_dict() {
    for (int i = 0; i < 256; i++) {
        dict[i] = malloc(2);
        dict[i][0] = (char)i;
        dict[i][1] = '\0';
    }
    for (int i = 256; i < MAX_DICT_SIZE; i++) {
        dict[i] = NULL;
    }
}

void free_dict() {
    for (int i = 0; i < MAX_DICT_SIZE; i++) {
        if (dict[i]) free(dict[i]);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <compressed_code_file>\n", argv[0]);
        return 1;
    }

    FILE* fp = fopen(argv[1], "r");
    if (!fp) {
        perror("File open failed");
        return 1;
    }

    init_dict();

    int prev_code, code;
    int next_code = 256;

    if (fscanf(fp, "%d", &prev_code) != 1) {
        printf("Input file is empty or invalid.\n");
        return 1;
    }

    printf("%d → '%s'\n", prev_code, dict[prev_code]);

    while (fscanf(fp, "%d", &code) == 1) {
        if (code < next_code && dict[code]) {
            printf("%d → '%s'\n", code, dict[code]);
        } else {
            // Special case: code not in dictionary yet
            printf("%d → (missing, assume '%s%c')\n", code, dict[prev_code], dict[prev_code][0]);
        }

        if (next_code < MAX_DICT_SIZE) {
            int len1 = strlen(dict[prev_code]);
            dict[next_code] = malloc(len1 + 2);
            strcpy(dict[next_code], dict[prev_code]);
            dict[next_code][len1] = dict[code < next_code && dict[code] ? code : prev_code][0];
            dict[next_code][len1 + 1] = '\0';

            printf("INSERT: %d → '%s'\n", next_code, dict[next_code]);
            next_code++;
        }

        prev_code = code;
    }

    fclose(fp);
    free_dict();
    return 0;
}
