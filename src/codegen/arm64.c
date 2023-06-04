#include <string.h>

#include "codegen.h"

#define arm64_x0 0
#define arm64_x1 1
#define arm64_x2 2
#define arm64_x3 3
#define arm64_x4 4
#define arm64_x5 5
#define arm64_x6 6
#define arm64_fp 29
#define arm64_lr 30
#define arm64_sp 31

#define arm64_inst(inst) *(codegen)->code_word_ptr++ = inst

void codegen_addr_arm64(Codegen *codegen, Node *node) {
    if (node->kind == NODE_LOCAL) {
        arm64_inst(0xD1000000 | ((node->local->offset & 0x1fff) << 10) | ((arm64_fp & 31) << 5) | (arm64_x0 & 31));  // sub x0, fp, imm
        return;
    }
    if (node->kind == NODE_DEREF) {
        codegen_node_arm64(codegen, node->lhs);
        return;
    }

    print_error(codegen->text, node->token, "Node is not an lvalue");
}

void codegen_node_arm64(Codegen *codegen, Node *node) {
    // Program
    if (node->kind == NODE_PROGRAM) {
        codegen->program = node;
        for (size_t i = 0; i < node->nodes.size; i++) {
            Node *function_node = node->nodes.items[i];
            codegen_node_arm64(codegen, function_node);
        }
    }

    // Function
    if (node->kind == NODE_FUNCTION) {
        codegen->current_function = node;

        // Set function ptr to current code offset
        node->function->address = (uint8_t *)codegen->code_word_ptr;

        // Set main address
        if (!strcmp(node->function->name, "main")) {
            codegen->main = (uint8_t *)codegen->code_word_ptr;
        }

        // Push link register
        if (!node->function->is_leaf) arm64_inst(0xF81F0FE0 | (arm64_lr & 31));  // str lr, [sp, -16]!

        // Allocate locals stack frame
        size_t aligned_locals_size = align(node->locals_size, 16);
        if (aligned_locals_size > 0) {
            arm64_inst(0xF81F0FE0 | (arm64_fp & 31));                                                                    // str fp, [sp, -16]!
            arm64_inst(0x910003FD);                                                                                      // mov fp, sp
            arm64_inst(0xD1000000 | ((aligned_locals_size & 0x1fff) << 10) | ((arm64_sp & 31) << 5) | (arm64_sp & 31));  // sub sp, sp, imm
        }

        // Write arguments to locals
        for (int32_t i = node->function->arguments.size - 1; i >= 0; i--) {
            Argument *argument = node->function->arguments.items[i];
            Local *local = node_find_local(node, argument->name);
            arm64_inst(0xD1000000 | ((local->offset & 0x1fff) << 10) | ((arm64_fp & 31) << 5) | (arm64_x6 & 31));  // sub x6, fp, imm
            if (i == 0) arm64_inst(0xF9000000 | ((arm64_x6 & 31) << 5) | (arm64_x0 & 31));                         // str x0, [x6]
            if (i == 1) arm64_inst(0xF9000000 | ((arm64_x6 & 31) << 5) | (arm64_x1 & 31));                         // str x1, [x6]
            if (i == 2) arm64_inst(0xF9000000 | ((arm64_x6 & 31) << 5) | (arm64_x2 & 31));                         // str x2, [x6]
            if (i == 3) arm64_inst(0xF9000000 | ((arm64_x6 & 31) << 5) | (arm64_x3 & 31));                         // str x3, [x6]
            if (i == 4) arm64_inst(0xF9000000 | ((arm64_x6 & 31) << 5) | (arm64_x4 & 31));                         // str x4, [x6]
            if (i == 5) arm64_inst(0xF9000000 | ((arm64_x6 & 31) << 5) | (arm64_x5 & 31));                         // str x5, [x6]
        }

        for (size_t i = 0; i < node->nodes.size; i++) {
            Node *child = node->nodes.items[i];
            codegen_node_arm64(codegen, child);
        }
        return;
    }

    // Nodes
    if (node->kind == NODE_NODES) {
        for (size_t i = 0; i < node->nodes.size; i++) {
            Node *child = node->nodes.items[i];
            codegen_node_arm64(codegen, child);
        }
        return;
    }

    // Statements
    if (node->kind == NODE_TENARY || node->kind == NODE_IF) {
        codegen_node_arm64(codegen, node->condition);

        uint32_t *else_label = codegen->code_word_ptr;
        arm64_inst(0);  // cbz x0, else

        codegen_node_arm64(codegen, node->then_block);

        uint32_t *done_label;
        if (node->else_block) {
            done_label = codegen->code_word_ptr;
            arm64_inst(0);  // b done
        }

        *else_label = 0xB4000000 | (((codegen->code_word_ptr - else_label) & 0x7ffff) << 5) | (arm64_x0 & 31);  // else:
        if (node->else_block) {
            codegen_node_arm64(codegen, node->else_block);

            *done_label = 0x14000000 | ((codegen->code_word_ptr - done_label) & 0x7ffffff);  // done:
        }
    }

    if (node->kind == NODE_WHILE) {
        uint32_t *loop_label = codegen->code_word_ptr;

        uint32_t *done_label;
        if (node->condition != NULL) {
            codegen_node_arm64(codegen, node->condition);
            done_label = codegen->code_word_ptr;
            arm64_inst(0);  // cbz x0, done
        }

        codegen_node_arm64(codegen, node->then_block);

        arm64_inst(0x14000000 | ((loop_label - codegen->code_word_ptr) & 0x7ffffff));  // b loop

        if (node->condition != NULL) {
            *done_label = 0xB4000000 | (((codegen->code_word_ptr - done_label) & 0x7ffff) << 5) | (arm64_x0 & 31);  // done:
        }
    }

    if (node->kind == NODE_DOWHILE) {
        uint32_t *loop_label = codegen->code_word_ptr;

        codegen_node_arm64(codegen, node->then_block);

        codegen_node_arm64(codegen, node->condition);
        arm64_inst(0xB5000000 | (((loop_label - codegen->code_word_ptr) & 0x7ffff) << 5) | (arm64_x0 & 31));  // cbnz x0, loop
    }

    if (node->kind == NODE_RETURN) {
        codegen_node_arm64(codegen, node->unary);

        // Free locals stack frame
        if (codegen->current_function->locals_size > 0) {
            arm64_inst(0x910003BF);                    // mov sp, fp
            arm64_inst(0xF84107E0 | (arm64_fp & 31));  // ldr fp, [sp], 16
        }

        // Push link register
        if (!codegen->current_function->function->is_leaf) arm64_inst(0xF84107E0 | (arm64_lr & 31));  // ldr fp, [sp], 16

        arm64_inst(0xD65F03C0);  // ret
        return;
    }

    // Unary
    if (node->kind == NODE_ADDR) {
        codegen_addr_arm64(codegen, node->unary);
        return;
    }

    if (node->kind == NODE_DEREF) {
        codegen_node_arm64(codegen, node->unary);

        // When array in array just return the pointer
        if (node->type->kind != TYPE_ARRAY) {
            arm64_inst(0xF9400000);  // ldr x0, [x0]
        }
        return;
    }

    if (node->kind > NODE_UNARY_BEGIN && node->kind < NODE_UNARY_END) {
        codegen_node_arm64(codegen, node->unary);

        if (node->kind == NODE_NEG) arm64_inst(0xCB0003E0);  // sub x0, xzr, x0
        if (node->kind == NODE_NOT) arm64_inst(0xAA2003E0);  // mvn x0, x0
        if (node->kind == NODE_LOGICAL_NOT) {
            arm64_inst(0xF100001F);  // cmp x0, 0
            arm64_inst(0x9A9F17E0);  // cset x0, eq
        }
        return;
    }

    // Operators
    if (node->kind == NODE_ASSIGN) {
        codegen_addr_arm64(codegen, node->lhs);
        arm64_inst(0xF81F0FE0 | (arm64_x0 & 31));  // str x0, [sp, -16]!

        codegen_node_arm64(codegen, node->rhs);
        arm64_inst(0xF84107E0 | (arm64_x1 & 31));  // ldr x1, [sp], 16

        arm64_inst(0xF9000020);  // str x0, [x1]
        return;
    }

    if (node->kind > NODE_OPERATION_BEGIN && node->kind < NODE_OPERATION_END) {
        codegen_node_arm64(codegen, node->rhs);
        arm64_inst(0xF81F0FE0 | (arm64_x0 & 31));  // str x0, [sp, -16]!

        codegen_node_arm64(codegen, node->lhs);
        arm64_inst(0xF84107E0 | (arm64_x1 & 31));  // ldr x1, [sp], 16

        if (node->kind == NODE_ADD) arm64_inst(0x8B010000);  // add x0, x0, x1
        if (node->kind == NODE_SUB) arm64_inst(0xCB010000);  // sub x0, x0, x1
        if (node->kind == NODE_MUL) arm64_inst(0x9B017C00);  // mul x0, x0, x1
        if (node->kind == NODE_DIV) arm64_inst(0x9AC10C00);  // sdiv x0, x0, x1
        if (node->kind == NODE_MOD) {
            arm64_inst(0x9AC10802);  // udiv x2, x0, x1
            arm64_inst(0x9B018040);  // msub x0, x2, x1, x0
        }
        if (node->kind == NODE_AND) arm64_inst(0x8A010000);  // and x0, x0, x1
        if (node->kind == NODE_OR) arm64_inst(0xAA010000);   // orr x0, x0, x1
        if (node->kind == NODE_XOR) arm64_inst(0xCA010000);  // eor x0, x0, x1
        if (node->kind == NODE_SHL) arm64_inst(0x9AC12000);  // lsl x0, x0, x1
        if (node->kind == NODE_SHR) arm64_inst(0x9AC12400);  // lsr x0, x0, x1

        if (node->kind > NODE_COMPARE_BEGIN && node->kind < NODE_COMPARE_END) {
            arm64_inst(0xEB01001F);                               // cmp x0, x1
            if (node->kind == NODE_EQ) arm64_inst(0x9A9F17E0);    // cset x0, eq
            if (node->kind == NODE_NEQ) arm64_inst(0x9A9F07E0);   // cset x0, ne
            if (node->kind == NODE_LT) arm64_inst(0x9A9FA7E0);    // cset x0, lt
            if (node->kind == NODE_LTEQ) arm64_inst(0x9A9FC7E0);  // cset x0, le
            if (node->kind == NODE_GT) arm64_inst(0x9A9FD7E0);    // cset x0, gt
            if (node->kind == NODE_GTEQ) arm64_inst(0x9A9FB7E0);  // cset x0, ge
        }

        if (node->kind == NODE_LOGICAL_AND || node->kind == NODE_LOGICAL_OR) {
            if (node->kind == NODE_LOGICAL_AND) arm64_inst(0x8A010000);  // and x0, x0, x1
            if (node->kind == NODE_LOGICAL_OR) arm64_inst(0xAA010000);   // orr x0, x0, x1
            arm64_inst(0xF100001F);                                      // cmp x0, 0
            arm64_inst(0x9A9F07E0);                                      // cset x0, ne
        }
        return;
    }

    // Values
    if (node->kind == NODE_LOCAL) {
        // When we load an array we don't load value because the array becomes a pointer
        if (node->local->type->kind == TYPE_ARRAY) {
            codegen_addr_arm64(codegen, node);
        } else {
            arm64_inst(0xD1000000 | ((node->local->offset & 0x1fff) << 10) | ((arm64_fp & 31) << 5) | (arm64_x1 & 31));  // sub x1, fp, imm
            arm64_inst(0xF9400020);                                                                                      // ldr x0, [x1]
        }
        return;
    }

    if (node->kind == NODE_INTEGER) {
        arm64_inst(0xD2800000 | ((node->integer & 0xffff) << 5) | (arm64_x0 & 31));  // mov x0, imm
        return;
    }

    if (node->kind == NODE_CALL) {
        // Write arguments to registers
        for (int32_t i = node->nodes.size - 1; i >= 0; i--) {
            Node *argument = node->nodes.items[i];
            codegen_node_arm64(codegen, argument);

            if (i == 1) arm64_inst(0xAA0003E0 | (arm64_x1 & 31));  // mov x1, x0
            if (i == 2) arm64_inst(0xAA0003E0 | (arm64_x2 & 31));  // mov x2, x0
            if (i == 3) arm64_inst(0xAA0003E0 | (arm64_x3 & 31));  // mov x3, x0
            if (i == 4) arm64_inst(0xAA0003E0 | (arm64_x4 & 31));  // mov x4, x0
            if (i == 5) arm64_inst(0xAA0003E0 | (arm64_x5 & 31));  // mov x5, x0
        }

        arm64_inst(0x94000000 | (((uint32_t *)node->function->address - codegen->code_word_ptr) & 0x7ffffff));  // bl function
        return;
    }
}
