#ifndef TYPE_H
#define TYPE_H

#include <stdbool.h>
#include <stdio.h>

typedef enum TypeKind { TYPE_NUMBER, TYPE_POINTER } TypeKind;

typedef struct Type Type;
struct Type {
    TypeKind kind;
    size_t size;
    bool isSigned;
    Type *base;
};

Type *type_new(TypeKind kind, size_t size, bool isSigned);

Type *type_pointer(Type *type);

void type_print(FILE *file, Type *type);

#endif
