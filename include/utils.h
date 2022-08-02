#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

typedef enum ArchKind { ARCH_ARM64, ARCH_X86_64 } ArchKind;

typedef struct Arch {
    ArchKind kind;
    int32_t wordSize;
    int32_t pointerSize;
    int32_t stackAlign;
    char **regs8;
    char **regs16;
    char **regs32;
    char **regs64;
    int32_t regsSize;
    int32_t *argRegs;
    int32_t argRegsSize;
    int32_t returnReg;
} Arch;

size_t align(size_t size, size_t alignment);

char *strdup(const char *string);

char *format(char *fmt, ...);

typedef struct List {
    void **items;
    size_t capacity;
    size_t size;
} List;

List *list_new(size_t capacity);

#define list_get(list, index) ((list)->items[index])

void list_add(List *list, void *item);

char *list_to_string(List *list);

#endif
