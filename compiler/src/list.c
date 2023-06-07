#include "list.h"
#include <stdlib.h>

List *list_new(void) { return list_new_with_capacity(0); }

List *list_new_with_capacity(size_t capacity) {
    List *list = calloc(1, sizeof(List));
    list->allocated = true;
    list->capacity = capacity;
    list_init(list);
    return list;
}

void list_init(List *list) {
    if (list->capacity == 0) list->capacity = 8;
    list->items = malloc(list->capacity * sizeof(void *));
}

void *list_get(List *list, size_t index) {
    if (index < list->size) return list->items[index];
    return NULL;
}

void list_set(List *list, size_t index, void *item) {
    if (index > list->capacity) {
        while (index > list->capacity) list->capacity <<= 1;
        list->items = realloc(list->items, list->capacity * sizeof(void *));
    }
    if (index > list->size) {
        for (size_t i = list->size; i < index - 1; i++) {
            list->items[i] = NULL;
        }
        list->size = index + 1;
    }
    list->items[index] = item;
}

void list_add(List *list, void *item) {
    if (list->size == list->capacity) {
        list->capacity <<= 1;
        list->items = realloc(list->items, list->capacity * sizeof(void *));
    }
    list->items[list->size++] = item;
}

void list_free(List *list, ListFreeFunc free_func) {
    if (free_func) {
        for (size_t i = 0; i < list->size; i++) {
            if (list->items[i]) free_func(list->items[i]);
        }
    }
    free(list->items);
    if (list->allocated) free(list);
}
