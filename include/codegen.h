#ifndef CODEGEN_H

#include "parser.h"

typedef enum Arch { ARCH_ARM64, ARCH_X86_64 } Arch;

void node_asm(FILE *file, Node *node, Node *next);

void codegen(FILE *file, Node *node, Arch arch);

#endif
