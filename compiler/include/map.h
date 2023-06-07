#ifndef MAP_H
#define MAP_H

#include <stdbool.h>
#include <stddef.h>

typedef struct Map {
    bool allocated;
    char **keys;
    void **values;
    size_t capacity;
    size_t filled;
} Map;

Map *map_new(void);

Map *map_new_with_capacity(size_t capacity);

void map_init(Map *map);

void *map_get(Map *map, char *key);

void map_set(Map *map, char *key, void *value);

typedef void (*MapFreeFunc)(void *);

void map_free(Map *map, MapFreeFunc free_func);

#endif
