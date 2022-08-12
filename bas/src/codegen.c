#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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

void codegen(Object *object, Node *node) {
    Codegen codegen;
    codegen.object = object;

    uint8_t *sectionBuffer = malloc(0x200);
    Section *section = section_new(SECTION_TEXT, ".text", sectionBuffer, 0);
    list_add(object->sections, section);
    codegen.currentSection = section;

    node_write(&codegen, node);
}

#define i8(imm) codegen->currentSection->data[codegen->currentSection->size++] = imm
#define i16(imm) *((int16_t *)&codegen->currentSection->data[codegen->currentSection->size]) = imm; codegen->currentSection->size += 2
#define i32(imm) *((int32_t *)&codegen->currentSection->data[codegen->currentSection->size]) = imm; codegen->currentSection->size += 4
#define i64(imm) *((int64_t *)&codegen->currentSection->data[codegen->currentSection->size]) = imm; codegen->currentSection->size += 8

void node_write(Codegen *codegen, Node *node) {
    if (node->kind == NODE_PROGRAM) {
        for (size_t i = 0; i < node->nodes->size; i++) {
            node_write(codegen, list_get(node->nodes, i));
        }
    }

    if (node->kind == NODE_TIMES) {
        int64_t times = node_calc(node->lhs);
        for (int64_t i = 0; i < times; i++) {
            node_write(codegen, node->rhs);
        }
    }

    if (node->kind == NODE_SECTION) {
        bool found = false;
        for (size_t i = 0; i < codegen->object->sections->size; i++) {
            Section *section = list_get(codegen->object->sections, i);
            if (!strcmp(section->name, node->string)) {
                found = true;
                codegen->currentSection = section;
                break;
            }
        }

        if (!found) {
            uint8_t *sectionBuffer = malloc(0x200);
            Section *section = section_new(SECTION_DATA, node->string, sectionBuffer, 0);
            list_add(codegen->object->sections, section);
            codegen->currentSection = section;
        }
    }

    if (node->kind == NODE_LABEL) {
        list_add(codegen->object->symbols, symbol_new(node->string, codegen->currentSection, codegen->currentSection->size));
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
                        i8(child->string[j]);
                        if (size > 1) codegen->currentSection->size += size - 1;
                    }
                } else {
                    int64_t answer = node_calc(child);
                    for (size_t j = 0; j < size; j++) {
                        i8(((uint8_t *)&answer)[j]);
                    }
                }
            }
        }

        if (node->opcode == TOKEN_X86_64_NOP) {
            i8(0x90);
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
            if (dest->size == 64) i8(0x48);
            if (dest->kind == NODE_REGISTER) {
                if (src->kind == NODE_REGISTER) {
                    i8(opcode);
                    i8((0b11 << 6) | (src->reg << 3) | dest->reg);
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

                    Node *srcreg = src->unary->lhs->kind == NODE_REGISTER ? src->unary->lhs : src->unary->rhs;
                    int64_t disp = node_is_calcable(src->unary->rhs) ? node_calc(src->unary->rhs) : node_calc(src->unary->lhs);
                    if (src->unary->kind == NODE_SUB) disp = -disp;
                    if (srcreg->size == 32) {
                        fprintf(stderr, "ERROR addr reg not 64\n");
                        exit(EXIT_FAILURE);
                    }

                    i8(node->opcode == TOKEN_X86_64_LEA ? opcode : (opcode | 0b10));
                    if (disp >= -128 && disp <= 127) {
                        i8((0b01 << 6) | (dest->reg << 3) | srcreg->reg);
                        i8(disp);
                    } else {
                        i8((0b10 << 6) | (dest->reg << 3) | srcreg->reg);
                        i32(disp);
                    }
                }

                if (node_is_calcable(src)) {
                    int64_t imm = node_calc(src);
                    if (node->opcode == TOKEN_X86_64_MOV) {
                        i8(0xb8 + dest->reg);
                        if (dest->size == 64) {
                            i64(imm);
                        } else {
                            i32(imm);
                        }
                    } else {
                        if (imm >= -128 && imm <= 127) {
                            i8(0x83);
                            i8((0b11 << 6) | (opcode2 << 3) | dest->reg);
                            i8(imm);
                        } else {
                            i8(0x81);
                            i8((0b11 << 6) | (opcode2 << 3) | dest->reg);
                            i32(imm);
                        }
                    }
                }
            }
            if (dest->kind == NODE_ADDR) {
                if (dest->unary->kind != NODE_ADD && dest->unary->kind != NODE_SUB) {
                    fprintf(stderr, "ERROR addr must contain add or sub\n");
                    exit(EXIT_FAILURE);
                }

                Node *destreg = dest->unary->lhs->kind == NODE_REGISTER ? dest->unary->lhs : dest->unary->rhs;
                int64_t disp = node_is_calcable(dest->unary->rhs) ? node_calc(dest->unary->rhs) : node_calc(dest->unary->lhs);
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

                    i8(opcode);
                    if (disp >= -128 && disp <= 127) {
                        i8((0b01 << 6) | (src->reg << 3) | destreg->reg);
                        i8(disp);
                    } else {
                        i8((0b10 << 6) | (src->reg << 3) | destreg->reg);
                        i32(disp);
                    }
                }
                if (node_is_calcable(src)) {
                    int64_t imm = node_calc(src);
                    i8((imm >= -128 && imm <= 127) ? (node->opcode == TOKEN_X86_64_MOV ? 0xc6 : 0x83) : (node->opcode == TOKEN_X86_64_MOV ? 0xc7 : 0x81));
                    if (disp >= -128 && disp <= 127) {
                        i8((0b01 << 6) | (opcode2 << 3) | destreg->reg);
                        i8(disp);
                    } else {
                        i8((0b10 << 6) | (opcode2 << 3) | destreg->reg);
                        i32(disp);
                    }
                    if (imm >= -128 && imm <= 127) {
                        i8(imm);
                    } else {
                        i32(imm);
                    }
                }
            }
        }

        if (node->opcode == TOKEN_X86_64_NEG) {
            Node *dest = (Node *)list_get(node->nodes, 0);
            if (dest->size == 64) i8(0x48);

            if (dest->kind == NODE_REGISTER) {
                i8(0xf7);
                i8((0b11 << 6) | (0b011 << 3) | dest->reg);
            }

            if (dest->kind == NODE_ADDR) {
                if (dest->unary->kind != NODE_ADD && dest->unary->kind != NODE_SUB) {
                    fprintf(stderr, "ERROR addr must contain add or sub\n");
                    exit(EXIT_FAILURE);
                }

                Node *destreg = dest->unary->lhs->kind == NODE_REGISTER ? dest->unary->lhs : dest->unary->rhs;
                int64_t disp = node_is_calcable(dest->unary->rhs) ? node_calc(dest->unary->rhs) : node_calc(dest->unary->lhs);
                if (dest->unary->kind == NODE_SUB) disp = -disp;
                if (destreg->size == 32) {
                    fprintf(stderr, "ERROR addr reg not 64\n");
                    exit(EXIT_FAILURE);
                }

                i8(0xf7);
                if (disp >= -128 && disp <= 127) {
                    i8((0b01 << 6) | (0b011 << 3) | destreg->reg);
                    i8(disp);
                } else {
                    i8((0b10 << 6) | (0b011 << 3) | destreg->reg);
                    i32(disp);
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

                i8(0x50 + src->reg);
            }
        }

        if (node->opcode == TOKEN_X86_64_POP) {
            Node *dest = (Node *)list_get(node->nodes, 0);
            if (dest->kind == NODE_REGISTER) {
                if (dest->size == 32) {
                    fprintf(stderr, "ERROR push not 64\n");
                    exit(EXIT_FAILURE);
                }

                i8(0x58 + dest->reg);
            }
        }

        if (node->opcode == TOKEN_X86_64_SYSCALL) {
            i8(0x0f);
            i8(0x05);
        }

        if (node->opcode == TOKEN_X86_64_CDQ) {
            i8(0x99);
        }

        if (node->opcode == TOKEN_X86_64_CQO) {
            i8(0x48);
            i8(0x99);
        }

        if (node->opcode == TOKEN_X86_64_LEAVE) {
            i8(0xc9);
        }

        if (node->opcode == TOKEN_X86_64_RET) {
            if (node->nodes->size > 0) {
                Node *unary = (Node *)list_get(node->nodes, 0);
                i8(0xc2);
                i16(node_calc(unary));
            } else {
                i8(0xc3);
            }
        }
    }
}
