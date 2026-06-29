#include <stdlib.h>

#include "../include/ft_malloc.h"

#include "../libft/include/ft_printf.h"

#define TINY_MAX 512
#define SMALL_MAX 4096

static size_t get_page_size_(void);

void *ft_malloc(size_t size) {
    ft_printf("Malloc: Not impl yet\n");
    (void)get_page_size_();

    if (size <= TINY_MAX) {
        ft_printf("Search for TINY\n");
    } else if (size <= SMALL_MAX) {
        ft_printf("Search for SMALL\n");
    } else {
        ft_printf("Alloc LARGE");
    }

    return NULL;
}

void *ft_realloc(void *ptr, size_t size) {
    ft_printf("Realloc: Not impl yet\n");

    (void)ptr;
    (void)size;

    return NULL;
}

void ft_free(void *ptr) {
    ft_printf("Free: Not impl yet\n");
    if (!ptr) {
        return;
    }
}

void show_alloc_mem(void) {
    ft_printf("Show: Not impl yet\n");
}

static size_t get_page_size_(void) {
#if defined(__APPLE__) && defined(__MACH__)
    return (size_t)getpagesize();
#elif defined(__linux__)
    long page_size = sysconf(_SC_PAGESIZE);

    if (page_size <= 0) {
        return 4096;
    }

    return (size_t)page_size;
#endif
}
