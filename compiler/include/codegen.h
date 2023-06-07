#ifndef CODEGEN_H
#define CODEGEN_H

#include "parser.h"
#include "utils.h"

// Codegen
typedef struct Codegen {
    Program *program;
    uint8_t *code_byte_ptr;
    uint32_t *code_word_ptr;
    Function *current_function;
} Codegen;

void codegen(Program *program);

// x86_64
void codegen_func_x86_64(Codegen *codegen, Function *function);

void codegen_stat_x86_64(Codegen *codegen, Node *node);

void codegen_addr_x86_64(Codegen *codegen, Node *node);

void codegen_expr_x86_64(Codegen *codegen, Node *node);

// arm64
void codegen_func_arm64(Codegen *codegen, Function *function);

void codegen_stat_arm64(Codegen *codegen, Node *node);

void codegen_addr_arm64(Codegen *codegen, Node *node);

void codegen_expr_arm64(Codegen *codegen, Node *node);

#endif
