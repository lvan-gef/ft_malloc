#include "./ft_free_list.h"

#include <assert.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>

#define DEFAULT_ALIGN alignof(max_align_t)

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

static void fl_all_(fl_list_t *fl);
static fl_node_t *fl_find_best_(fl_list_t *fl, size_t size, size_t align,
                                size_t *padding, fl_node_t **prev_node);

static void fl_node_insert_(fl_node_t **head, fl_node_t *prev_node,
                            fl_node_t *new_node);
static void fl_node_remove_(fl_node_t **head, fl_node_t *prev_node,
                            fl_node_t *del_node);
static void fl_coalescence_(fl_list_t *fl, fl_node_t *prev_node,
                            fl_node_t *free_node);
static size_t align_up_(size_t value, size_t align);
static size_t calc_padding_(uintptr_t ptr, uintptr_t align, size_t header_size);

fl_list_t *fl_init(size_t size) {

    size_t meta_size = align_up_(sizeof(fl_list_t), sizeof(void *));
    size_t total_size = meta_size + size;
    void *mem = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        return NULL;
    }

    fl_list_t *fl = (fl_list_t *)mem;
    fl->data = (char *)mem + meta_size;
    fl->size = size;
    fl->full_size = total_size;
    fl->used = 0;
    fl_all_(fl);

    return fl;
}

void *fl_alloc(fl_list_t *fl, size_t size) {
    size_t padding = 0;
    fl_node_t *cur_node = NULL;
    fl_node_t *prev_node = NULL;
    fl_header_t *header = NULL;

    if (size < sizeof(fl_node_t)) {
        size = sizeof(fl_node_t);
    }

    cur_node = fl_find_best_(fl, size, DEFAULT_ALIGN, &padding, &prev_node);
    if (!cur_node) {
        return NULL;
    }

    size_t align_padding = padding - sizeof(fl_header_t);
    size_t required_space = size + padding;
    size_t remaining = cur_node->block_size - required_space;

    if (remaining >= sizeof(fl_node_t)) {
        fl_node_t *new_node = (fl_node_t *)((char *)cur_node + required_space);
        new_node->block_size = remaining;
        fl_node_insert_(&fl->head, cur_node, new_node);
    } else {
        required_space = cur_node->block_size;
    }

    fl_node_remove_(&fl->head, prev_node, cur_node);

    header = (fl_header_t *)((char *)cur_node + align_padding);
    header->block_size = required_space;
    header->padding = align_padding;

    fl->used += required_space;
    return (void *)((char *)header + sizeof(fl_header_t));
}

void fl_free(fl_list_t *fl, void *ptr) {
    if (!ptr) {
        return;
    }

    fl_header_t *header = (fl_header_t *)((char *)ptr - sizeof(fl_header_t));
    fl_node_t *free_node = (fl_node_t *)((char *)header - header->padding);
    free_node->block_size = header->block_size;
    free_node->next = NULL;

    fl_node_t *cur_node = fl->head;
    fl_node_t *prev_node = NULL;
    while (cur_node) {
        if ((uintptr_t)free_node < (uintptr_t)cur_node) {
            break;
        }

        prev_node = cur_node;
        cur_node = cur_node->next;
    }
    fl_node_insert_(&fl->head, prev_node, free_node);
    fl->used -= free_node->block_size;
    fl_coalescence_(fl, prev_node, free_node);
}

static void fl_all_(fl_list_t *fl) {
    fl_node_t *first_node = (fl_node_t *)fl->data;
    first_node->block_size = fl->size;
    first_node->next = NULL;
    fl->head = first_node;
}

static fl_node_t *fl_find_best_(fl_list_t *fl, size_t size, size_t align,
                                size_t *padding_, fl_node_t **prev_node_) {
    size_t diff = SIZE_MAX;
    fl_node_t *cur_node = fl->head;
    fl_node_t *prev_node = NULL;
    fl_node_t *best_node = NULL;
    fl_node_t *best_prev_node = NULL;

    size_t padding = 0;
    size_t best_padding = 0;
    while (cur_node) {
        padding = calc_padding_((uintptr_t)cur_node, (uintptr_t)align,
                                sizeof(fl_header_t));
        if (padding > SIZE_MAX - size) {
            prev_node = cur_node;
            cur_node = cur_node->next;
            continue;
        }
        size_t required_space = size + padding;

        if (cur_node->block_size >= required_space &&
            cur_node->block_size - required_space < diff) {
            best_node = cur_node;
            best_prev_node = prev_node;
            diff = cur_node->block_size - required_space;
            best_padding = padding;
        }

        prev_node = cur_node;
        cur_node = cur_node->next;
    }

    if (padding_) {
        *padding_ = best_padding;
    }

    if (prev_node_) {
        *prev_node_ = best_prev_node;
    }

    return best_node;
}

static void fl_node_insert_(fl_node_t **head, fl_node_t *prev_node,
                            fl_node_t *new_node) {
    if (!prev_node) {
        new_node->next = *head;
        *head = new_node;
    } else {
        new_node->next = prev_node->next;
        prev_node->next = new_node;
    }
}

static void fl_node_remove_(fl_node_t **head, fl_node_t *prev_node,
                            fl_node_t *del_node) {
    if (!prev_node) {
        *head = del_node->next;
    } else {
        prev_node->next = del_node->next;
    }
}

static void fl_coalescence_(fl_list_t *fl, fl_node_t *prev_node,
                            fl_node_t *free_node) {
    if (free_node->next != NULL &&
        (void *)((char *)free_node + free_node->block_size) ==
            free_node->next) {
        free_node->block_size += free_node->next->block_size;
        fl_node_remove_(&fl->head, free_node, free_node->next);
    }

    if (prev_node &&
        (void *)((char *)prev_node + prev_node->block_size) == free_node) {
        prev_node->block_size += free_node->block_size;
        fl_node_remove_(&fl->head, prev_node, free_node);
    }
}

static size_t align_up_(size_t value, size_t align) {
    return ((value + align - 1) & ~(align - 1));
}

static size_t calc_padding_(uintptr_t ptr, uintptr_t align,
                            size_t header_size) {
    assert(align > 0 && "align must be more then 0");
    assert((align & (align - 1)) == 0 && "align must be a power of two");
    uintptr_t p = ptr;
    uintptr_t a = align;
    uintptr_t modulo = p & (a - 1);
    uintptr_t padding = 0;

    if (modulo) {
        padding = a - modulo;
    }

    uintptr_t needed_space = (uintptr_t)header_size;
    if (padding < needed_space) {
        needed_space -= padding;

        if ((needed_space & (a - 1))) {
            padding += a * (1 + (needed_space / a));
        } else {
            padding += a * (needed_space / a);
        }
    }

    return (size_t)padding;
}
