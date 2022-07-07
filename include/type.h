#ifndef TYPE_H
#define TYPE_H

#include <stdio.h>

typedef enum TypeKind { TYPE_NUMBER, TYPE_POINTER } TypeKind;

typedef struct Type Type;
struct Type {
    TypeKind kind;
    Type *base;
};

Type *type_new(TypeKind kind);

Type *type_pointer(Type *type);

void type_print(FILE *file, Type *type);

#endif
