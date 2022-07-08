#ifndef TYPE_H
#define TYPE_H

#include <stdbool.h>
#include <stdio.h>

typedef enum TypeKind { TYPE_INTEGER, TYPE_POINTER, TYPE_ARRAY } TypeKind;

typedef struct Type Type;
struct Type {
    TypeKind kind;
    size_t size;
    bool isSigned;
    Type *base;
    size_t count;
};

Type *type_new(TypeKind kind, size_t size, bool isSigned);

Type *type_base(Type *type);

Type *type_pointer(Type *type);

Type *type_array(Type *type, size_t count);

void type_print(FILE *file, Type *type);

#endif
