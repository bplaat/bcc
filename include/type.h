#ifndef TYPE_H
#define TYPE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum TypeKind { TYPE_INTEGER, TYPE_POINTER, TYPE_ARRAY } TypeKind;

typedef struct Type Type;
struct Type {
    TypeKind kind;
    int32_t size;
    bool isSigned;
    Type *base;
    int32_t count;
};

Type *type_new(TypeKind kind, int32_t size, bool isSigned);

Type *type_new_pointer(Type *type);

Type *type_new_array(Type *type, int32_t count);

Type *type_base(Type *type);

bool type_is_32(Type *type);

bool type_is_64(Type *type);

char *type_to_string(Type *type);

#endif
