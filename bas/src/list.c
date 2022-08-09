#include "list.h"
#include <stdlib.h>
#include <string.h>

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

char *list_to_string(List *list) {
    size_t size = 0;
    for (size_t i = 0; i < list->size; i++) {
        size += strlen(list_get(list, i));
    }
    char *string = malloc(size + 1);
    string[0] = '\0';
    for (size_t i = 0; i < list->size; i++) {
        strcat(string, list_get(list, i));
    }
    return string;
}

List *text_to_list(char *text) {
    List *lines = list_new(128);
    char *c = text;
    list_add(lines, c);
    while (*c != '\0') {
        if (*c == '\n' || *c == '\r') {
            if (*c == '\r') {
                *c = '\0';
                c++;
            } else {
                *c = '\0';
            }
            c++;
            list_add(lines, c);
        } else {
            c++;
        }
    }
    return lines;
}
