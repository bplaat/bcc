#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void IMM16(uint8_t **end, int16_t imm) {
    int16_t *c = (int16_t *)*end;
    *c++ = imm;
    *end = (uint8_t *)c;
}

void IMM32(uint8_t **end, int32_t imm) {
    int32_t *c = (int32_t *)*end;
    *c++ = imm;
    *end = (uint8_t *)c;
}

void IMM64(uint8_t **end, int64_t imm) {
    int64_t *c = (int64_t *)*end;
    *c++ = imm;
    *end = (uint8_t *)c;
}

int64_t node_calc(Node *node) {
    if (node->kind == NODE_INTEGER) {
        return node->integer;
    }
    if (node->kind == NODE_ADD) {
        return node_calc(node->lhs) + node_calc(node->rhs);
    }
    if (node->kind == NODE_SUB) {
        return node_calc(node->lhs) - node_calc(node->rhs);
    }
    if (node->kind == NODE_MUL) {
        return node_calc(node->lhs) * node_calc(node->rhs);
    }
    if (node->kind == NODE_DIV) {
        return node_calc(node->lhs) / node_calc(node->rhs);
    }
    if (node->kind == NODE_MOD) {
        return node_calc(node->lhs) % node_calc(node->rhs);
    }
    return 0;
}

void node_write(Arch *arch, uint8_t **end, Node *node) {
    uint8_t *c = *end;

    if (node->kind == NODE_PROGRAM) {
        for (size_t i = 0; i < node->nodes->size; i++) {
            node_write(arch, &c, list_get(node->nodes, i));
        }
    }

    if (node->kind == NODE_TIMES) {
        for (int64_t i = 0; i < node->lhs->integer; i++) {
            node_write(arch, &c, node->rhs);
        }
    }

    if (node->kind == NODE_INSTRUCTION) {
        if (node->opcode >= TOKEN_DB && node->opcode <= TOKEN_DQ) {
            size_t size = 1;
            if (node->opcode == TOKEN_DW) size = 2;
            if (node->opcode == TOKEN_DD) size = 4;
            if (node->opcode == TOKEN_DQ) size = 8;
            for (size_t i = 0; i < node->nodes->size; i++) {
                Node *child = list_get(node->nodes, i);
                if (child->kind == NODE_STRING) {
                    for (size_t j = 0; j < strlen(child->string); j++) {
                        *c++ = child->string[j];
                        if (size != 1) c += size - 1;
                    }
                } else {
                    int64_t answer = node_calc(child);
                    for (size_t j = 0; j < size; j++) {
                        *c++ = ((uint8_t *)&answer)[j];
                    }
                }
            }
        }

        if (node->opcode == TOKEN_X86_64_NOP) {
            *c++ = 0x90;
        }

        if (node->opcode >= TOKEN_X86_64_MOV && node->opcode <= TOKEN_X86_64_XOR) {
            Node *dest = (Node *)list_get(node->nodes, 0);
            Node *src = (Node *)list_get(node->nodes, 1);

            int32_t opcode, opcode2;
            if (node->opcode == TOKEN_X86_64_MOV) opcode = 0x89, opcode2 = 0b000;
            if (node->opcode == TOKEN_X86_64_LEA) opcode = 0x8d, opcode2 = 0b000;
            if (node->opcode == TOKEN_X86_64_ADD) opcode = 0x01, opcode2 = 0b000;
            if (node->opcode == TOKEN_X86_64_ADC) opcode = 0x11, opcode2 = 0b010;
            if (node->opcode == TOKEN_X86_64_SUB) opcode = 0x29, opcode2 = 0b101;
            if (node->opcode == TOKEN_X86_64_SBB) opcode = 0x19, opcode2 = 0b011;
            if (node->opcode == TOKEN_X86_64_CMP) opcode = 0x39, opcode2 = 0b111;
            if (node->opcode == TOKEN_X86_64_AND) opcode = 0x21, opcode2 = 0b100;
            if (node->opcode == TOKEN_X86_64_OR) opcode = 0x09, opcode2 = 0b001;
            if (node->opcode == TOKEN_X86_64_XOR) opcode = 0x31, opcode2 = 0b110;
            if (dest->size == 64) *c++ = 0x48;
            if (dest->kind == NODE_REGISTER) {
                if (src->kind == NODE_REGISTER) {
                    *c++ = opcode;
                    *c++ = (0b11 << 6) | (src->reg << 3) | dest->reg;
                }

                if (src->kind == NODE_ADDR) {
                    if (dest->size != src->size) {
                        fprintf(stderr, "ERROR addr size and dest reg are not the same\n");
                        exit(EXIT_FAILURE);
                    }
                    if (src->unary->kind != NODE_ADD && src->unary->kind != NODE_SUB) {
                        fprintf(stderr, "ERROR addr must contain add or sub\n");
                        exit(EXIT_FAILURE);
                    }

                    Node *srcreg = src->unary->lhs;
                    int64_t disp = src->unary->rhs->integer;
                    if (src->unary->kind == NODE_SUB) disp = -disp;
                    if (srcreg->size == 32) {
                        fprintf(stderr, "ERROR addr reg not 64\n");
                        exit(EXIT_FAILURE);
                    }

                    *c++ = node->opcode == TOKEN_X86_64_LEA ? opcode : (opcode | 0b10);
                    if (disp >= -128 && disp <= 127) {
                        *c++ = (0b01 << 6) | (dest->reg << 3) | srcreg->reg;
                        *c++ = disp;
                    } else {
                        *c++ = (0b10 << 6) | (dest->reg << 3) | srcreg->reg;
                        IMM32(&c, disp);
                    }
                }

                if (src->kind == NODE_INTEGER) {
                    if (node->opcode == TOKEN_X86_64_MOV) {
                        *c++ = 0xb8 + dest->reg;
                        if (dest->size == 64) {
                            IMM64(&c, src->integer);
                        } else {
                            IMM32(&c, src->integer);
                        }
                    } else {
                        *c++ = src->integer >= -128 && src->integer <= 127 ? 0x83 : 0x81;
                        *c++ = (0b11 << 6) | (opcode2 << 3) | dest->reg;
                        if (src->integer >= -128 && src->integer <= 127) {
                            *c++ = src->integer;
                        } else {
                            IMM32(&c, src->integer);
                        }
                    }
                }
            }
            if (dest->kind == NODE_ADDR) {
                if (dest->unary->kind != NODE_ADD && dest->unary->kind != NODE_SUB) {
                    fprintf(stderr, "ERROR addr must contain add or sub\n");
                    exit(EXIT_FAILURE);
                }

                Node *destreg = dest->unary->lhs;
                int64_t disp = dest->unary->rhs->integer;
                if (dest->unary->kind == NODE_SUB) disp = -disp;
                if (destreg->size == 32) {
                    fprintf(stderr, "ERROR addr reg not 64\n");
                    exit(EXIT_FAILURE);
                }

                if (src->kind == NODE_REGISTER) {
                    if (dest->size != src->size) {
                        fprintf(stderr, "ERROR addr size and dest reg are not the same\n");
                        exit(EXIT_FAILURE);
                    }

                    *c++ = opcode;
                    if (disp >= -128 && disp <= 127) {
                        *c++ = (0b01 << 6) | (src->reg << 3) | destreg->reg;
                        *c++ = disp;
                    } else {
                        *c++ = (0b10 << 6) | (src->reg << 3) | destreg->reg;
                        IMM32(&c, disp);
                    }
                }
                if (src->kind == NODE_INTEGER) {
                    *c++ = (src->integer >= -128 && src->integer <= 127) ? (node->opcode == TOKEN_X86_64_MOV ? 0xc6 : 0x83) : (node->opcode == TOKEN_X86_64_MOV ? 0xc7 : 0x81);
                    if (disp >= -128 && disp <= 127) {
                        *c++ = (0b01 << 6) | (opcode2 << 3) | destreg->reg;
                        *c++ = disp;
                    } else {
                        *c++ = (0b10 << 6) | (opcode2 << 3) | destreg->reg;
                        IMM32(&c, disp);
                    }
                    if (src->integer >= -128 && src->integer <= 127) {
                        *c++ = src->integer;
                    } else {
                        IMM32(&c, src->integer);
                    }
                }
            }
        }

        if (node->opcode == TOKEN_X86_64_NEG) {
            Node *dest = (Node *)list_get(node->nodes, 0);
            if (dest->size == 64) *c++ = 0x48;

            if (dest->kind == NODE_REGISTER) {
                *c++ = 0xf7;
                *c++ = (0b11 << 6) | (0b011 << 3) | dest->reg;
            }

            if (dest->kind == NODE_ADDR) {
                if (dest->unary->kind != NODE_ADD && dest->unary->kind != NODE_SUB) {
                    fprintf(stderr, "ERROR addr must contain add or sub\n");
                    exit(EXIT_FAILURE);
                }

                Node *destreg = dest->unary->lhs;
                int64_t disp = dest->unary->rhs->integer;
                if (dest->unary->kind == NODE_SUB) disp = -disp;
                if (destreg->size == 32) {
                    fprintf(stderr, "ERROR addr reg not 64\n");
                    exit(EXIT_FAILURE);
                }

                *c++ = 0xf7;
                if (disp >= -128 && disp <= 127) {
                    *c++ = (0b01 << 6) | (0b011 << 3) | destreg->reg;
                    *c++ = disp;
                } else {
                    *c++ = (0b10 << 6) | (0b011 << 3) | destreg->reg;
                    IMM32(&c, disp);
                }
            }
        }

        if (node->opcode == TOKEN_X86_64_PUSH) {
            Node *src = (Node *)list_get(node->nodes, 0);
            if (src->kind == NODE_REGISTER) {
                if (src->size == 32) {
                    fprintf(stderr, "ERROR push not 64\n");
                    exit(EXIT_FAILURE);
                }

                *c++ = 0x50 + src->reg;
            }
        }

        if (node->opcode == TOKEN_X86_64_POP) {
            Node *dest = (Node *)list_get(node->nodes, 0);
            if (dest->kind == NODE_REGISTER) {
                if (dest->size == 32) {
                    fprintf(stderr, "ERROR push not 64\n");
                    exit(EXIT_FAILURE);
                }

                *c++ = 0x58 + dest->reg;
            }
        }

        if (node->opcode == TOKEN_X86_64_SYSCALL) {
            *c++ = 0x0f;
            *c++ = 0x05;
        }

        if (node->opcode == TOKEN_X86_64_CDQ) {
            *c++ = 0x99;
        }

        if (node->opcode == TOKEN_X86_64_CQO) {
            *c++ = 0x48;
            *c++ = 0x99;
        }

        if (node->opcode == TOKEN_X86_64_LEAVE) {
            *c++ = 0xc9;
        }

        if (node->opcode == TOKEN_X86_64_RET) {
            if (node->nodes->size > 0) {
                Node *imm = (Node *)list_get(node->nodes, 0);
                *c++ = 0xc2;
                IMM16(&c, imm->integer);
            } else {
                *c++ = 0xc3;
            }
        }
    }

    *end = c;
}
