#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

size_t align(size_t size, size_t alignment);

typedef struct List {
    void **items;
    size_t capacity;
    size_t size;
} List;

List *list_new(size_t capacity);

#define list_get(list, index) ((list)->items[index])

void list_add(List *list, void *item);

#endif
