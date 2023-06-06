#include <stdlib.h>
#include <string.h>

#include "codegen.h"

#define x0 0
#define x1 1
#define x2 2
#define x3 3
#define x4 4
#define x5 5
#define x6 6
#define fp 29
#define lr 30
#define sp 31

#define inst(inst)                      \
    {                                   \
        *codegen->code_word_ptr = inst; \
        codegen->code_word_ptr++;       \
    }

void codegen_func_arm64(Codegen *codegen, Function *function) {
    codegen->current_function = function;

    // Set function ptr to current code offset
    function->address = (uint8_t *)codegen->code_word_ptr;

    // Set main address
    if (!strcmp(function->name, "main")) {
        codegen->program->main_func = codegen->code_word_ptr;
    }

    // Push link register
    if (!function->is_leaf) inst(0xF81F0FE0 | (lr & 31));  // str lr, [sp, -16]!

    // Allocate locals stack frame
    size_t aligned_locals_size = align(function->locals_size, 16);
    if (aligned_locals_size > 0) {
        inst(0xF81F0FE0 | (fp & 31));                                                              // str fp, [sp, -16]!
        inst(0x910003FD);                                                                          // mov fp, sp
        inst(0xD1000000 | ((aligned_locals_size & 0x1fff) << 10) | ((sp & 31) << 5) | (sp & 31));  // sub sp, sp, imm
    }

    // Write arguments to locals
    for (int32_t i = function->arguments.size - 1; i >= 0; i--) {
        Argument *argument = function->arguments.items[i];
        Local *local = function_find_local(function, argument->name);

        inst(0xD1000000 | ((local->offset & 0x1fff) << 10) | ((fp & 31) << 5) | (x6 & 31));  // sub x6, fp, imm
        if (i == 0) {
            if (local->type->size == 1) inst(0x3D000000 | ((x6 & 31) << 5) | (x0 & 31));  // str b0, [x6]
            if (local->type->size == 2) inst(0x7D000000 | ((x6 & 31) << 5) | (x0 & 31));  // str w0, [x6]
            if (local->type->size == 4) inst(0xB9000000 | ((x6 & 31) << 5) | (x0 & 31));  // str h0, [x6]
            if (local->type->size == 8) inst(0xF9000000 | ((x6 & 31) << 5) | (x0 & 31));  // str x0, [x6]
        }
        if (i == 1) {
            if (local->type->size == 1) inst(0x3D000000 | ((x6 & 31) << 5) | (x1 & 31));  // str b1, [x6]
            if (local->type->size == 2) inst(0x7D000000 | ((x6 & 31) << 5) | (x1 & 31));  // str w1, [x6]
            if (local->type->size == 4) inst(0xB9000000 | ((x6 & 31) << 5) | (x1 & 31));  // str h1, [x6]
            if (local->type->size == 8) inst(0xF9000000 | ((x6 & 31) << 5) | (x1 & 31));  // str x1, [x6]
        }
        if (i == 2) {
            if (local->type->size == 1) inst(0x3D000000 | ((x6 & 31) << 5) | (x2 & 31));  // str b2, [x6]
            if (local->type->size == 2) inst(0x7D000000 | ((x6 & 31) << 5) | (x2 & 31));  // str w2, [x6]
            if (local->type->size == 4) inst(0xB9000000 | ((x6 & 31) << 5) | (x2 & 31));  // str h2, [x6]
            if (local->type->size == 8) inst(0xF9000000 | ((x6 & 31) << 5) | (x2 & 31));  // str x2, [x6]
        }
        if (i == 3) {
            if (local->type->size == 1) inst(0x3D000000 | ((x6 & 31) << 5) | (x3 & 31));  // str b3, [x6]
            if (local->type->size == 2) inst(0x7D000000 | ((x6 & 31) << 5) | (x3 & 31));  // str w3, [x6]
            if (local->type->size == 4) inst(0xB9000000 | ((x6 & 31) << 5) | (x3 & 31));  // str h3, [x6]
            if (local->type->size == 8) inst(0xF9000000 | ((x6 & 31) << 5) | (x3 & 31));  // str x3, [x6]
        }
    }

    for (size_t i = 0; i < function->nodes.size; i++) {
        Node *child = function->nodes.items[i];
        codegen_stat_arm64(codegen, child);
    }
}

void codegen_stat_arm64(Codegen *codegen, Node *node) {
    // Nodes
    if (node->kind == NODE_NODES) {
        for (size_t i = 0; i < node->nodes.size; i++) {
            Node *child = node->nodes.items[i];
            codegen_stat_arm64(codegen, child);
        }
        return;
    }

    // Statements
    if (node->kind == NODE_IF) {
        codegen_expr_arm64(codegen, node->condition);

        uint32_t *else_label = codegen->code_word_ptr;
        inst(0);  // cbz x0, else

        codegen_stat_arm64(codegen, node->then_block);

        uint32_t *done_label;
        if (node->else_block) {
            done_label = codegen->code_word_ptr;
            inst(0);  // b done
        }

        *else_label = 0xB4000000 | (((codegen->code_word_ptr - else_label) & 0x7ffff) << 5) | (x0 & 31);  // else:
        if (node->else_block) {
            codegen_stat_arm64(codegen, node->else_block);

            *done_label = 0x14000000 | ((codegen->code_word_ptr - done_label) & 0x7ffffff);  // done:
        }
        return;
    }

    if (node->kind == NODE_WHILE) {
        uint32_t *loop_label = codegen->code_word_ptr;

        uint32_t *done_label;
        if (node->condition != NULL) {
            codegen_expr_arm64(codegen, node->condition);
            done_label = codegen->code_word_ptr;
            inst(0);  // cbz x0, done
        }

        codegen_stat_arm64(codegen, node->then_block);

        inst(0x14000000 | ((loop_label - codegen->code_word_ptr) & 0x7ffffff));  // b loop

        if (node->condition != NULL) {
            *done_label = 0xB4000000 | (((codegen->code_word_ptr - done_label) & 0x7ffff) << 5) | (x0 & 31);  // done:
        }
        return;
    }

    if (node->kind == NODE_DOWHILE) {
        uint32_t *loop_label = codegen->code_word_ptr;

        codegen_stat_arm64(codegen, node->then_block);

        codegen_expr_arm64(codegen, node->condition);
        inst(0xB5000000 | (((loop_label - codegen->code_word_ptr) & 0x7ffff) << 5) | (x0 & 31));  // cbnz x0, loop
        return;
    }

    if (node->kind == NODE_RETURN) {
        codegen_expr_arm64(codegen, node->unary);

        // Free locals stack frame
        if (codegen->current_function->locals_size > 0) {
            inst(0x910003BF);              // mov sp, fp
            inst(0xF84107E0 | (fp & 31));  // ldr fp, [sp], 16
        }

        // Push link register
        if (!codegen->current_function->is_leaf) inst(0xF84107E0 | (lr & 31));  // ldr fp, [sp], 16

        inst(0xD65F03C0);  // ret
        return;
    }

    codegen_expr_arm64(codegen, node);
}

void codegen_addr_arm64(Codegen *codegen, Node *node) {
    if (node->kind == NODE_GLOBAL) {
        inst(0x10000000 | ((((uint32_t *)node->global->address - codegen->code_word_ptr) & 0x7ffff) << 5) | (x0 & 31));  // adr x1, imm
        return;
    }
    if (node->kind == NODE_LOCAL) {
        inst(0xD1000000 | ((node->local->offset & 0x1fff) << 10) | ((fp & 31) << 5) | (x0 & 31));  // sub x0, fp, imm
        return;
    }
    if (node->kind == NODE_DEREF) {
        codegen_expr_arm64(codegen, node->lhs);
        return;
    }

    print_error(node->token, "Node is not an lvalue");
}

void codegen_expr_arm64(Codegen *codegen, Node *node) {
    // Tenary
    if (node->kind == NODE_TENARY || node->kind == NODE_IF) {
        codegen_expr_arm64(codegen, node->condition);

        uint32_t *else_label = codegen->code_word_ptr;
        inst(0);  // cbz x0, else

        codegen_expr_arm64(codegen, node->then_block);

        uint32_t *done_label = codegen->code_word_ptr;
        inst(0);  // b done

        *else_label = 0xB4000000 | (((codegen->code_word_ptr - else_label) & 0x7ffff) << 5) | (x0 & 31);  // else:

        codegen_expr_arm64(codegen, node->else_block);

        *done_label = 0x14000000 | ((codegen->code_word_ptr - done_label) & 0x7ffffff);  // done:
        return;
    }

    // Unary
    if (node->kind == NODE_ADDR) {
        codegen_addr_arm64(codegen, node->unary);
        return;
    }

    if (node->kind == NODE_DEREF) {
        codegen_expr_arm64(codegen, node->unary);

        // When array in array just return the pointer
        if (node->type->kind != TYPE_ARRAY) {
            inst(0xF9400000);  // ldr x0, [x0]
        }
        return;
    }

    if (node->kind > NODE_UNARY_BEGIN && node->kind < NODE_UNARY_END) {
        codegen_expr_arm64(codegen, node->unary);

        if (node->kind == NODE_NEG) inst(0xCB0003E0);  // sub x0, xzr, x0
        if (node->kind == NODE_NOT) inst(0xAA2003E0);  // mvn x0, x0
        if (node->kind == NODE_LOGICAL_NOT) {
            inst(0xF100001F);  // cmp x0, 0
            inst(0x9A9F17E0);  // cset x0, eq
        }
        return;
    }

    // Operators
    if (node->kind == NODE_ASSIGN) {
        codegen_addr_arm64(codegen, node->lhs);
        inst(0xF81F0FE0 | (x0 & 31));  // str x0, [sp, -16]!

        codegen_expr_arm64(codegen, node->rhs);
        inst(0xF84107E0 | (x1 & 31));  // ldr x1, [sp], 16

        Type *type = node->lhs->type;
        if (type->size == 1) inst(0x3D000020);  // str b0, [x1]
        if (type->size == 2) inst(0x7D000020);  // str h0, [x1]
        if (type->size == 4) inst(0xB9000020);  // str w0, [x1]
        if (type->size == 8) inst(0xF9000020);  // str x0, [x1]
        return;
    }

    if (node->kind > NODE_OPERATION_BEGIN && node->kind < NODE_OPERATION_END) {
        codegen_expr_arm64(codegen, node->rhs);
        inst(0xF81F0FE0 | (x0 & 31));  // str x0, [sp, -16]!

        codegen_expr_arm64(codegen, node->lhs);
        inst(0xF84107E0 | (x1 & 31));  // ldr x1, [sp], 16

        if (node->kind == NODE_ADD) inst(0x8B010000);  // add x0, x0, x1
        if (node->kind == NODE_SUB) inst(0xCB010000);  // sub x0, x0, x1
        if (node->kind == NODE_MUL) inst(0x9B017C00);  // mul x0, x0, x1
        if (node->kind == NODE_DIV) inst(0x9AC10C00);  // sdiv x0, x0, x1
        if (node->kind == NODE_MOD) {
            inst(0x9AC10802);  // udiv x2, x0, x1
            inst(0x9B018040);  // msub x0, x2, x1, x0
        }
        if (node->kind == NODE_AND) inst(0x8A010000);  // and x0, x0, x1
        if (node->kind == NODE_OR) inst(0xAA010000);   // orr x0, x0, x1
        if (node->kind == NODE_XOR) inst(0xCA010000);  // eor x0, x0, x1
        if (node->kind == NODE_SHL) inst(0x9AC12000);  // lsl x0, x0, x1
        if (node->kind == NODE_SHR) inst(0x9AC12400);  // lsr x0, x0, x1

        if (node->kind > NODE_COMPARE_BEGIN && node->kind < NODE_COMPARE_END) {
            inst(0xEB01001F);                               // cmp x0, x1
            if (node->kind == NODE_EQ) inst(0x9A9F17E0);    // cset x0, eq
            if (node->kind == NODE_NEQ) inst(0x9A9F07E0);   // cset x0, ne
            if (node->kind == NODE_LT) inst(0x9A9FA7E0);    // cset x0, lt
            if (node->kind == NODE_LTEQ) inst(0x9A9FC7E0);  // cset x0, le
            if (node->kind == NODE_GT) inst(0x9A9FD7E0);    // cset x0, gt
            if (node->kind == NODE_GTEQ) inst(0x9A9FB7E0);  // cset x0, ge
        }

        if (node->kind == NODE_LOGICAL_AND || node->kind == NODE_LOGICAL_OR) {
            if (node->kind == NODE_LOGICAL_AND) inst(0x8A010000);  // and x0, x0, x1
            if (node->kind == NODE_LOGICAL_OR) inst(0xAA010000);   // orr x0, x0, x1
            inst(0xF100001F);                                      // cmp x0, 0
            inst(0x9A9F07E0);                                      // cset x0, ne
        }
        return;
    }

    // Values
    if (node->kind == NODE_GLOBAL) {
        // When we load an array we don't load value because the array becomes a pointer
        Type *type = node->global->type;
        if (type->kind == TYPE_ARRAY) {
            codegen_addr_arm64(codegen, node);
        } else {
            inst(0x10000000 | ((((uint32_t *)node->global->address - codegen->code_word_ptr) & 0x7ffff) << 5) | (x1 & 31));  // adr x1, imm
            if (type->size == 1) inst(0x3D400020);                                                                           // ldr b0, [x1]
            if (type->size == 2) inst(0x7D400020);                                                                           // ldr h0, [x1]
            if (type->size == 4) inst(0xB9400020);                                                                           // ldr w0, [x1]
            if (type->size == 8) inst(0xF9400020);                                                                           // ldr x0, [x1]
        }
        return;
    }
    if (node->kind == NODE_LOCAL) {
        // When we load an array we don't load value because the array becomes a pointer
        Type *type = node->local->type;
        if (type->kind == TYPE_ARRAY) {
            codegen_addr_arm64(codegen, node);
        } else {
            inst(0xD1000000 | ((node->local->offset & 0x1fff) << 10) | ((fp & 31) << 5) | (x1 & 31));  // sub x1, fp, imm
            if (type->size == 1) inst(0x3D400020);                                                     // ldr b0, [x1]
            if (type->size == 2) inst(0x7D400020);                                                     // ldr h0, [x1]
            if (type->size == 4) inst(0xB9400020);                                                     // ldr w0, [x1]
            if (type->size == 8) inst(0xF9400020);                                                     // ldr x0, [x1]
        }
        return;
    }

    if (node->kind == NODE_INTEGER) {
        inst(0xD2800000 | ((node->integer & 0xffff) << 5) | (x0 & 31));  // mov x0, imm
        return;
    }

    if (node->kind == NODE_CALL) {
        // Write arguments to registers
        for (int32_t i = node->nodes.size - 1; i >= 0; i--) {
            Node *argument = node->nodes.items[i];
            codegen_expr_arm64(codegen, argument);

            if (i == 1) inst(0xAA0003E0 | (x1 & 31));  // mov x1, x0
            if (i == 2) inst(0xAA0003E0 | (x2 & 31));  // mov x2, x0
            if (i == 3) inst(0xAA0003E0 | (x3 & 31));  // mov x3, x0
        }

        inst(0x94000000 | (((uint32_t *)node->function->address - codegen->code_word_ptr) & 0x7ffffff));  // bl function
        return;
    }

    print_error(node->token, "Unknown node kind");
    exit(EXIT_FAILURE);
}
