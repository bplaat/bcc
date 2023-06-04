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
    char *text;
    void *code;
    uint8_t *code_byte_ptr;
    uint32_t *code_word_ptr;
    void *main;

    Node *program;
    Node *current_function;
} Codegen;

void codegen(Arch arch, void *code, char *text, Node *node, void **main);

void codegen_addr_x86_64(Codegen *codegen, Node *node);

void codegen_node_x86_64(Codegen *codegen, Node *node);

void codegen_addr_arm64(Codegen *codegen, Node *node);

void codegen_node_arm64(Codegen *codegen, Node *node);

#endif
