#include "map.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

static uint32_t map_hash(char *key) {
    uint32_t hash = 2166136261;
    while (*key) {
        hash ^= *key++;
        hash *= 16777619;
    }
    return hash;
}

Map *map_new(void) { return map_new_with_capacity(0); }

Map *map_new_with_capacity(size_t capacity) {
    Map *map = calloc(1, sizeof(Map));
    map->allocated = true;
    map->capacity = capacity;
    map_init(map);
    return map;
}

void map_init(Map *map) {
    if (map->capacity == 0) map->capacity = 8;
    map->keys = calloc(map->capacity, sizeof(char *));
    map->values = malloc(map->capacity * sizeof(void *));
}

void *map_get(Map *map, char *key) {
    size_t index = map_hash(key) & (map->capacity - 1);
    while (map->keys[index]) {
        if (!strcmp(map->keys[index], key)) {
            return map->values[index];
        }
        index = (index + 1) & (map->capacity - 1);
    }
    return NULL;
}

void map_set(Map *map, char *key, void *value) {
    if (map->filled >= map->capacity * 3 / 4) {
        map->capacity <<= 1;
        char **newKeys = calloc(map->capacity, sizeof(char *));
        void **newValues = malloc(map->capacity * sizeof(void *));
        for (size_t i = 0; i < map->capacity >> 1; i++) {
            if (map->keys[i]) {
                size_t index = map_hash(map->keys[i]) & (map->capacity - 1);
                while (newKeys[index]) {
                    index = (index + 1) & (map->capacity - 1);
                }
                newKeys[index] = map->keys[i];
                newValues[index] = map->values[i];
            }
        }
        free(map->keys);
        free(map->values);
        map->keys = newKeys;
        map->values = newValues;
    }

    size_t index = map_hash(key) & (map->capacity - 1);
    while (map->keys[index]) {
        if (!strcmp(map->keys[index], key)) {
            map->values[index] = value;
            return;
        }
        index = (index + 1) & (map->capacity - 1);
    }

    map->keys[index] = strdup(key);
    map->values[index] = value;
    map->filled++;
}

void map_free(Map *map, MapFreeFunc free_func) {
    for (size_t i = 0; i < map->capacity; i++) {
        if (map->keys[i]) free(map->keys[i]);
        if (free_func && map->values[i]) free_func(map->values[i]);
    }

    free(map->keys);
    free(map->values);
    if (map->allocated) free(map);
}
