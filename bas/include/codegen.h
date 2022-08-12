#pragma once

#include "object.h"
#include "node.h"

typedef struct Var {
    char *name;
    int64_t value;
} Var;

Var *var_new(char *name, int64_t value);

typedef struct Codegen {
    Object *object;
    Section *currentSection;
    List *vars;
} Codegen;

void codegen(Object *object, Node *node);

int64_t node_execute(Codegen *codegen, Node *node);

void codegen_node(Codegen *codegen, Node *node);
