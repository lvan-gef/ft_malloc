#ifndef FT_MALLOC_H
#define FT_MALLOC_H

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

void *malloc(size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

void show_alloc_mem(void);

#endif // !FT_MALLOC_H
