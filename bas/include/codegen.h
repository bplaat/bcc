#pragma once

#include "object.h"
#include "node.h"

void IMM16(uint8_t **end, int16_t imm);

void IMM32(uint8_t **end, int32_t imm);

void IMM64(uint8_t **end, int64_t imm);

int64_t node_calc(Node *node);

void node_write(Arch *arch, uint8_t **end, Node *node);
