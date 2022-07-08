#include "type.h"

#include <stdbool.h>
#include <stdlib.h>

Type *type_new(TypeKind kind, size_t size, bool isSigned) {
    Type *type = malloc(sizeof(Type));
    type->kind = kind;
    type->size = size;
    type->isSigned = isSigned;
    type->base = NULL;
    return type;
}

Type *type_base(Type *type) {
    if (type->base != NULL) {
        return type->base;
    }
    return type;
}

Type *type_pointer(Type *type) {
    Type *pointerType = malloc(sizeof(Type));
    pointerType->kind = TYPE_POINTER;
    pointerType->size = 8;
    pointerType->isSigned = false;
    pointerType->base = type;
    return pointerType;
}

Type *type_array(Type *type, size_t count) {
    Type *pointerType = malloc(sizeof(Type));
    pointerType->kind = TYPE_ARRAY;
    pointerType->size = type->size * count;
    pointerType->isSigned = type->isSigned;
    pointerType->base = type;
    pointerType->count = count;
    return pointerType;
}

void _type_print(FILE *file, Type *type) {
    if (type->kind == TYPE_INTEGER) {
        if (type->isSigned) {
            fprintf(file, "i%lu", type->size * 8);
        } else {
            fprintf(file, "u%lu", type->size * 8);
        }
    }

    if (type->kind == TYPE_POINTER) {
        _type_print(file, type->base);
        fprintf(file, "*");
    }

    if (type->kind == TYPE_ARRAY) {
        _type_print(file, type->base);
        fprintf(file, "[%zu]", type->count);
    }
}

void type_print(FILE *file, Type *type) {
    fprintf(file, "[");
    _type_print(file, type);
    fprintf(file, "]");
}
