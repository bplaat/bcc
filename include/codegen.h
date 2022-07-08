#ifndef CODEGEN_H

#include "parser.h"
#include "utils.h"

void node_asm(FILE *file, Node *parent, Node *node, Node *next);

void codegen(FILE *file, Node *node, Arch *arch);

#endif
