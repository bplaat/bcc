#include "type.h"

#include <stdbool.h>
#include <stdlib.h>

Type *type_new(TypeKind kind, size_t size, bool isSigned) {
    Type *type = malloc(sizeof(Type));
    type->kind = kind;
    type->size = size;
    type->isSigned = isSigned;
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


void _type_print(FILE *file, Type *type, bool enclose) {
    if (enclose) fprintf(file, "[");

    if (type->kind == TYPE_NUMBER) {
        if (type->isSigned) {
            fprintf(file, "i%lu", type->size * 8);
        } else {
            fprintf(file, "u%lu", type->size * 8);
        }
    }

    if (type->kind == TYPE_POINTER) {
        fprintf(file, "*");
        _type_print(file, type->base, false);
    }

    if (enclose) fprintf(file, "]");
}

void type_print(FILE *file, Type *type) {
    _type_print(file, type, true);
}
