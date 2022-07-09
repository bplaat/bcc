#include "utils.h"
#include <string.h>

size_t align(size_t size, size_t alignment) { return (size + alignment - 1) / alignment * alignment; }

char *strdup(const char *string) {
    char *newString = malloc(strlen(string) + 1);
    strcpy(newString, string);
    return newString;
}

List *list_new(size_t capacity) {
    List *list = malloc(sizeof(List));
    list->items = malloc(sizeof(void *) * capacity);
    list->capacity = capacity;
    list->size = 0;
    return list;
}

void list_add(List *list, void *item) {
    if (list->size == list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, sizeof(void *) * list->capacity);
    }
    list->items[list->size++] = item;
}
