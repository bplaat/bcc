#include "type.h"

#include <stdbool.h>
#include <stdlib.h>

Type *type_new(TypeKind kind) {
    Type *type = malloc(sizeof(Type));
    type->kind = kind;
    return type;
}

Type *type_pointer(Type *type) {
    Type *pointerType = malloc(sizeof(Type));
    pointerType->kind = TYPE_POINTER;
    pointerType->base = type;
    return pointerType;
}

void type_print(FILE *file, Type *type) {
    static bool dontEnclose = false;
    if (!dontEnclose) fprintf(file, "[");
    if (type->kind == TYPE_NUMBER) {
        fprintf(file, "i64");
    }
    if (type->kind == TYPE_POINTER) {
        fprintf(file, "*");
        dontEnclose = true;
        type_print(file, type->base);
        dontEnclose = false;
    }
    if (!dontEnclose) fprintf(file, "]");
}
