#include "type.h"

#include <stdlib.h>

Type *type_new(TypeKind kind, int32_t size, bool isUnsigned) {
    Type *type = malloc(sizeof(Type));
    type->kind = kind;
    type->size = size;
    type->isUnsigned = isUnsigned;
    return type;
}

Type *type_new_pointer(Type *baseType) {
    Type *type = type_new(TYPE_POINTER, 8, true);
    type->base = baseType;
    return type;
}

Type *type_new_array(Type *baseType, int32_t count) {
    Type *type = type_new(TYPE_ARRAY, count * baseType->size, true);
    type->base = baseType;
    type->count = count;
    return type;
}

Type *type_base(Type *type) {
    if (type->kind == TYPE_POINTER || type->kind == TYPE_ARRAY) {
        return type->base;
    }
    return type;
}

bool type_is_8(Type *type) { return type->size == 1; }

bool type_is_16(Type *type) { return type->size == 2; }

bool type_is_32(Type *type) { return type->size == 4; }

bool type_is_64(Type *type) { return type->kind == TYPE_ARRAY || type->size == 8; }

char *type_to_string(Type *type) {
    if (type->kind == TYPE_INTEGER) {
        return format("%c%d", type->isUnsigned ? 'u' : 'i', type->size * 8);
    }
    if (type->kind == TYPE_POINTER) {
        return format("%s*", type_to_string(type->base));
    }
    if (type->kind == TYPE_ARRAY) {
        return format("%s[%d]", type_to_string(type->base), type->count);
    }
    return NULL;
}
