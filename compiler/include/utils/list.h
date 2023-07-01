#ifndef LIST_H
#define LIST_H

#include <stdbool.h>
#include <stddef.h>

typedef struct List {
    bool allocated;
    void **items;
    size_t capacity;
    size_t size;
} List;

List *list_new(void);

List *list_new_with_capacity(size_t capacity);

void list_init(List *list);

void *list_get(List *list, size_t index);

void list_set(List *list, size_t index, void *item);

void list_add(List *list, void *item);

typedef void (*ListFreeFunc)(void *);

void list_free(List *list, ListFreeFunc free_func);

#endif
