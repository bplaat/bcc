#pragma once

#include "object.h"
#include "node.h"

void IMM16(uint8_t **end, int16_t imm);

void IMM32(uint8_t **end, int32_t imm);

void IMM64(uint8_t **end, int64_t imm);

int64_t node_calc(Node *node);

typedef struct Codegen {
    Object *object;
    Section *currentSection;
} Codegen;

void codegen(Object *object, Node *node);

void node_write(Codegen *codegen, Node *node);
