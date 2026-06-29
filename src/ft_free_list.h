#ifndef FT_FREE_LIST_H
#define FT_FREE_LIST_H

#include <stddef.h>

typedef struct fl_header_s {
    size_t block_size;
    size_t padding;
} fl_header_t;

typedef struct fl_node_s {
    struct fl_node_s *next;
    size_t block_size;
} fl_node_t;

typedef struct fl_list_s {
    void *data;
    size_t size;
    size_t full_size;
    size_t used;
    fl_node_t *head;
} fl_list_t;

fl_list_t *fl_init(size_t size);
void *fl_alloc(fl_list_t *fl, size_t size);
void fl_free(fl_list_t *fl, void *ptr);
void fl_destroy(fl_list_t *fl);

#endif // !FT_FREE_LIST_H
