#ifndef CODEGEN_H
#define CODEGEN_H

#include "parser.h"
#include "utils.h"

// Arch
typedef enum Arch {
    ARCH_X86_64,
    ARCH_ARM64,
} Arch;

// Codegen
typedef struct Codegen {
    void *code;
    uint8_t *code_byte_ptr;
    uint32_t *code_word_ptr;
} Codegen;

void codegen(Arch arch, void *code, Node *node);

void codegen_node_x86_64(Codegen *codegen, Node *node);

void codegen_node_arm64(Codegen *codegen, Node *node);

#endif
