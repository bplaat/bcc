#ifndef CODEGEN_H

#include <stdio.h>

#include "parser.h"
#include "utils.h"

typedef struct Codegen {
    FILE *file;
    Arch *arch;
    bool *regsUsed;
    Node *currentFuncdef;
    int32_t uniqueLabel;
    bool nestedAssign;
} Codegen;

void codegen(FILE *file, Arch *arch, Node *node);

int32_t codegen_alloc(Codegen *codegen, int32_t requestReg);

void codegen_free(Codegen *codegen, int32_t reg);

int32_t codegen_part(Codegen *codegen, Node *node, int32_t requestReg);

#endif
