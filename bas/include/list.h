#pragma once

#include <stddef.h>

typedef struct List {
    void **items;
    size_t capacity;
    size_t size;
} List;

List *list_new(size_t capacity);

#define list_get(list, index) ((list)->items[index])

void list_add(List *list, void *item);

char *list_to_string(List *list);

List *text_to_list(char *text);
