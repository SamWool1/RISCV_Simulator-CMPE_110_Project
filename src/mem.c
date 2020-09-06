//
// Created by javaprophet on 10/20/18.
//

#include "mem.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

void* smalloc(size_t n) {
    void* raw = malloc(n);
    if (n > 0 && raw == NULL) {
        printf("Out of memory.\n");
        exit(1);
    }
    return raw;
}

void* scalloc(size_t n) {
    void* raw = calloc(n, 1);
    if (n > 0 && raw == NULL) {
        printf("Out of memory.\n");
        exit(1);
    }
    return raw;
}

void* srealloc(void* original, size_t n) {
    void* raw = realloc(original, n);
    if (n > 0 && raw == NULL) {
        printf("Out of memory.\n");
        exit(1);
    }
    return raw;
}
