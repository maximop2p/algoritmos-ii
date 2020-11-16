#include "strutil.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_IND 100

char *substr(const char *str, size_t n) {
    char* c = calloc(n + 1, sizeof(char));
    if (!c) return NULL;
    strncpy(c, str, n);
    return c;
}

char **split(const char *str, char sep) {
    if (!str) return NULL;
    size_t n = strlen(str);
    size_t cadenas = 0;
    char** cc = NULL;
    size_t indices[MAX_IND];
    indices[0] = 0;
    for (size_t i = 0; i < n; i++) {
        if (str[i] == sep) {
            cadenas++;
            indices[cadenas] = i + 1;
        }
    }
    indices[++cadenas] = n+ 1;
    cc = malloc(sizeof(char*) * (cadenas + 1));
    if (!cc) return NULL;
    for (size_t i = 0; i < cadenas; i++) {
        cc[i] = substr(str + indices[i], indices[i + 1] - indices[i] - 1);
    }
    cc[cadenas] = NULL;
    return cc;
}

char *join(char **strv, char sep) {
    size_t strs = 0, mem = 0;
    if (!strv) return NULL;

    size_t lens[MAX_IND];
    for (size_t i = 0; strv[i]; i++) {
        lens[strs] = strlen(strv[i]) + 1;
        mem += lens[strs++];
    }
    if (!mem) {
        char* c = calloc(1, sizeof(char));
        if (!c) return NULL;
        c[0] = "\0";
        return c;
    }
    char* c = calloc(mem, sizeof(char));

    if (!c) return NULL;
    size_t index = 0;
    for (size_t i = 0; i < strs; i++) {
        strncpy(c + index, strv[i], lens[i]);
        index += lens[i] -1;
        strncpy(c + index++, &sep, 1);
    }
    c[index-1] = '\0';
    return c;
}

void free_strv(char *strv[]) {
    for (size_t i = 0; strv[i]; i++)
        free(strv[i]);
    free(strv);
}
