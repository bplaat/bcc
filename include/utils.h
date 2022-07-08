#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

typedef enum ArchKind { ARCH_ARM64, ARCH_X86_64 } ArchKind;

typedef struct Arch {
    ArchKind kind;
    int32_t stackAlign;
    char **argumentRegs;
} Arch;

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
