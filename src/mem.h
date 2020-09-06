
#ifndef RISCVSIM_MEM_H
#define RISCVSIM_MEM_H

#include <stdlib.h>

void* smalloc(size_t n);

void* scalloc(size_t n);

void* srealloc(void* original, size_t n);

#endif //RISCVSIM_MEM_H
