#ifndef CODEGEN_H

#include "parser.h"

typedef enum Arch {
    ARCH_ARM64,
    ARCH_X86_64
} Arch;

extern Arch arch;

void node_asm(FILE *file, Node *node);

#endif
