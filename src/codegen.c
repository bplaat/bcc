#include <codegen.h>

void codegen(Arch arch, void *code, Node *node) {
    Codegen codegen = {
        .code = code,
        .code_byte_ptr = (uint8_t *)code,
        .code_word_ptr = (uint32_t *)code,
    };
    if (arch == ARCH_X86_64) codegen_node_x86_64(&codegen, node);
    if (arch == ARCH_ARM64) codegen_node_arm64(&codegen, node);
}

// x86_64
#define x86_64_rax 0
#define x86_64_rcx 1
#define x86_64_rdx 2
#define x86_64_rbx 3
#define x86_64_rsp 4
#define x86_64_rbp 5

#define x86_64_inst1(a) *(codegen)->code_byte_ptr++ = a
#define x86_64_inst2(a, b)               \
    {                                    \
        x86_64_inst1(a);                 \
        *(codegen)->code_byte_ptr++ = b; \
    }
#define x86_64_inst3(a, b, c)            \
    {                                    \
        x86_64_inst2(a, b);              \
        *(codegen)->code_byte_ptr++ = c; \
    }
#define x86_64_inst4(a, b, c, d)         \
    {                                    \
        x86_64_inst3(a, b, c);           \
        *(codegen)->code_byte_ptr++ = d; \
    }
#define x86_64_imm32(imm)                           \
    {                                               \
        *((int32_t *)codegen->code_byte_ptr) = imm; \
        codegen->code_byte_ptr += sizeof(int32_t);  \
    }
#define x86_64_imm64(imm)                           \
    {                                               \
        *((int64_t *)codegen->code_byte_ptr) = imm; \
        codegen->code_byte_ptr += sizeof(int64_t);  \
    }

void codegen_node_x86_64(Codegen *codegen, Node *node) {
    if (node->type == NODE_FUNCTION) {
        Node *oldFunction = codegen->currentFunction;
        codegen->currentFunction = node;

        // Calculate locals size
        size_t aligned_locals_size = align(node->locals_size, 16);

        // Allocate locals stack frame
        if (aligned_locals_size > 0) {
            x86_64_inst1(0x50 | (x86_64_rbp & 7));  // push rbp
            x86_64_inst3(0x48, 0x89, 0xe5);         // mov rbp, rsp
            x86_64_inst3(0x48, 0x81, 0xec);         // sub rsp, imm
            x86_64_imm32(aligned_locals_size);
        }

        for (size_t i = 0; i < node->nodes.size; i++) {
            Node *child = node->nodes.items[i];
            codegen_node_x86_64(codegen, child);
        }

        // Free locals stack frame
        if (aligned_locals_size > 0) {
            x86_64_inst3(0x48, 0x89, 0xec);         // mov rsp, rbp
            x86_64_inst1(0x58 | (x86_64_rbp & 7));  // pop rbp
        }
        x86_64_inst1(0xc3);  // ret

        codegen->currentFunction = oldFunction;
        return;
    }

    if (node->type == NODE_NODES) {
        for (size_t i = 0; i < node->nodes.size; i++) {
            Node *child = node->nodes.items[i];
            codegen_node_x86_64(codegen, child);
        }
        return;
    }

    if (node->type == NODE_LOCAL) {
        x86_64_inst3(0x48, 0x8b, 0x85);  // mov rax, qword [rbp - imm]
        x86_64_imm32(-node->local->address);
        return;
    }

    if (node->type == NODE_INTEGER) {
        if (node->integer > INT32_MIN && node->integer < INT32_MAX) {
            x86_64_inst1(0xb8 | (x86_64_rax & 7));  // mov eax, imm
            x86_64_imm32(node->integer);
        } else {
            x86_64_inst2(0x48, 0xb8 | (x86_64_rax & 7));  // movabs rax, imm
            x86_64_imm64(node->integer);
        }
        return;
    }

    if (node->type == NODE_IF) {
        codegen_node_x86_64(codegen, node->condition);
        x86_64_inst4(0x48, 0x83, 0xf8, 0x00);  // cmp rax, 0
        x86_64_inst2(0x0f, 0x84);              // je else
        uint8_t *else_label = codegen->code_byte_ptr;
        x86_64_imm32(0);

        codegen_node_x86_64(codegen, node->thenBlock);

        *((int32_t *)else_label) = codegen->code_byte_ptr - (else_label + sizeof(int32_t));  // else:
        if (node->elseBlock) {
            codegen_node_x86_64(codegen, node->elseBlock);
        }
    }

    if (node->type == NODE_WHILE) {
        uint8_t *loop_label = codegen->code_byte_ptr;

        uint8_t *done_label;
        if (node->condition != NULL) {
            codegen_node_x86_64(codegen, node->condition);
            x86_64_inst4(0x48, 0x83, 0xf8, 0x00);  // cmp rax, 0
            x86_64_inst2(0x0f, 0x84);              // je done
            done_label = codegen->code_byte_ptr;
            x86_64_imm32(0);
        }

        codegen_node_x86_64(codegen, node->thenBlock);

        x86_64_inst1(0xe9);  // jmp loop
        x86_64_imm32(loop_label - (codegen->code_byte_ptr + sizeof(int32_t)));

        if (node->condition != NULL) {
            *((int32_t *)done_label) = codegen->code_byte_ptr - (done_label + sizeof(int32_t));  // done:
        }
    }

    if (node->type == NODE_DOWHILE) {
        uint8_t *loop_label = codegen->code_byte_ptr;

        codegen_node_x86_64(codegen, node->thenBlock);

        codegen_node_x86_64(codegen, node->condition);
        x86_64_inst4(0x48, 0x83, 0xf8, 0x00);  // cmp rax, 0
        x86_64_inst2(0x0f, 0x85);              // jne loop
        x86_64_imm32(loop_label - (codegen->code_byte_ptr + sizeof(int32_t)));
    }

    if (node->type == NODE_RETURN) {
        codegen_node_x86_64(codegen, node->unary);

        if (codegen->currentFunction->locals.filled > 0) {
            x86_64_inst3(0x48, 0x89, 0xec);         // mov rsp, rbp
            x86_64_inst1(0x58 | (x86_64_rbp & 7));  // pop rbp
        }
        x86_64_inst1(0xc3);  // ret
        return;
    }

    if (node->type > NODE_UNARY_BEGIN && node->type < NODE_UNARY_END) {
        codegen_node_x86_64(codegen, node->unary);

        if (node->type == NODE_NEG) x86_64_inst3(0x48, 0xf7, 0xd8);  // neg rax
        if (node->type == NODE_NOT) x86_64_inst3(0x48, 0xf7, 0xd0);  // not rax
        if (node->type == NODE_LOGICAL_NOT) {
            x86_64_inst4(0x48, 0x83, 0xf8, 0x00);  // cmp rax, 0
            x86_64_inst3(0x0f, 0x94, 0xc0);        // sete al
            x86_64_inst4(0x48, 0x0f, 0xb6, 0xc0);  // movzx rax, al
        }
        return;
    }

    if (node->type == NODE_ASSIGN) {
        codegen_node_x86_64(codegen, node->rhs);

        x86_64_inst3(0x48, 0x89, 0x85);  // mov qword [rbp - imm], rax
        x86_64_imm32(-node->lhs->local->address);
        return;
    }

    if (node->type > NODE_OPERATION_BEGIN && node->type < NODE_OPERATION_END) {
        codegen_node_x86_64(codegen, node->rhs);
        x86_64_inst1(0x50 | (x86_64_rax & 7));  // push rax

        codegen_node_x86_64(codegen, node->lhs);
        x86_64_inst1(0x58 | (x86_64_rcx & 7));  // pop rcx

        if (node->type == NODE_ADD) x86_64_inst3(0x48, 0x01, 0xc8);        // add rax, rcx
        if (node->type == NODE_SUB) x86_64_inst3(0x48, 0x29, 0xc8);        // sub rax, rcx
        if (node->type == NODE_MUL) x86_64_inst4(0x48, 0x0f, 0xaf, 0xc1);  // imul rax, rcx
        if (node->type == NODE_DIV) {
            x86_64_inst2(0x48, 0x99);        // cqo
            x86_64_inst3(0x48, 0xf7, 0xf9);  // idiv rcx
        }
        if (node->type == NODE_MOD) {
            x86_64_inst2(0x48, 0x99);        // cqo
            x86_64_inst3(0x48, 0xf7, 0xf9);  // idiv rcx
            x86_64_inst3(0x48, 0x89, 0xd0);  // mov rax, rdx
        }
        if (node->type == NODE_AND) x86_64_inst3(0x48, 0x21, 0xc8);  // and rax, rcx
        if (node->type == NODE_OR) x86_64_inst3(0x48, 0x09, 0xc8);   // or rax, rcx
        if (node->type == NODE_XOR) x86_64_inst3(0x48, 0x31, 0xc8);  // xor rax, rcx
        if (node->type == NODE_SHL) x86_64_inst3(0x48, 0xd3, 0xe0);  // shl rax, cl
        if (node->type == NODE_SHR) x86_64_inst3(0x48, 0xd3, 0xe8);  // shr rax, cl

        if (node->type > NODE_COMPARE_BEGIN && node->type < NODE_COMPARE_END) {
            x86_64_inst3(0x48, 0x39, 0xc8);                               // cmp rax, rcx
            if (node->type == NODE_EQ) x86_64_inst3(0x0f, 0x94, 0xc0);    // sete al
            if (node->type == NODE_NEQ) x86_64_inst3(0x0f, 0x95, 0xc0);   // setne al
            if (node->type == NODE_LT) x86_64_inst3(0x0f, 0x9c, 0xc0);    // setl al
            if (node->type == NODE_LTEQ) x86_64_inst3(0x0f, 0x9e, 0xc0);  // setle al
            if (node->type == NODE_GT) x86_64_inst3(0x0f, 0x9f, 0xc0);    // setg al
            if (node->type == NODE_GTEQ) x86_64_inst3(0x0f, 0x9d, 0xc0);  // setge al
            x86_64_inst4(0x48, 0x0f, 0xb6, 0xc0);                         // movzx rax, al
        }

        if (node->type == NODE_LOGICAL_AND || node->type == NODE_LOGICAL_OR) {
            if (node->type == NODE_LOGICAL_AND) x86_64_inst3(0x48, 0x21, 0xc8);  // and rax, rcx
            if (node->type == NODE_LOGICAL_OR) x86_64_inst3(0x48, 0x09, 0xc8);   // or rax, rcx
            x86_64_inst4(0x48, 0x83, 0xf8, 0x00);                                // cmp rax, 0
            x86_64_inst3(0x0f, 0x95, 0xc0);                                      // setne al
            x86_64_inst4(0x48, 0x0f, 0xb6, 0xc0);                                // movzx rax, al
        }
        return;
    }
}

// arm64
#define arm64_x0 0
#define arm64_x1 1
#define arm64_x2 2
#define arm64_fp 29
#define arm64_sp 31

#define arm64_inst(inst) *(codegen)->code_word_ptr++ = inst

void codegen_node_arm64(Codegen *codegen, Node *node) {
    if (node->type == NODE_FUNCTION) {
        Node *oldFunction = codegen->currentFunction;
        codegen->currentFunction = node;

        // Calculate locals size
        size_t aligned_locals_size = align(node->locals_size, 16);

        // Allocate locals stack frame
        if (aligned_locals_size > 0) {
            arm64_inst(0xF81F0FE0 | (arm64_fp & 31));                                                                    // str fp, [sp, -16]!
            arm64_inst(0x910003FD);                                                                                      // mov fp, sp
            arm64_inst(0xD1000000 | ((aligned_locals_size & 0x1fff) << 10) | ((arm64_sp & 31) << 5) | (arm64_sp & 31));  // sub sp, sp, imm
        }

        for (size_t i = 0; i < node->nodes.size; i++) {
            Node *child = node->nodes.items[i];
            codegen_node_arm64(codegen, child);
        }

        // Free locals stack frame
        if (aligned_locals_size > 0) {
            arm64_inst(0x910003BF);                    // mov sp, fp
            arm64_inst(0xF84107E0 | (arm64_fp & 31));  // ldr fp, [sp], 16
        }
        arm64_inst(0xD65F03C0);  // ret

        codegen->currentFunction = oldFunction;
        return;
    }

    if (node->type == NODE_NODES) {
        for (size_t i = 0; i < node->nodes.size; i++) {
            Node *child = node->nodes.items[i];
            codegen_node_arm64(codegen, child);
        }
        return;
    }

    if (node->type == NODE_LOCAL) {
        arm64_inst(0xD1000000 | ((node->local->address & 0x1fff) << 10) | ((arm64_fp & 31) << 5) | (arm64_x1 & 31));  // sub x1, fp, imm
        arm64_inst(0xF9400020);                                                                                       // ldr x0, [x1]
        return;
    }

    if (node->type == NODE_INTEGER) {
        arm64_inst(0xD2800000 | ((node->integer & 0xffff) << 5) | (arm64_x0 & 31));  // mov x0, imm
        return;
    }

    if (node->type == NODE_IF) {
        codegen_node_arm64(codegen, node->condition);

        uint32_t *else_label = codegen->code_word_ptr;
        arm64_inst(0);  // cbz x0, else

        codegen_node_arm64(codegen, node->thenBlock);

        *else_label = 0xB4000000 | (((codegen->code_word_ptr - else_label) & 0x7ffff) << 5) | (arm64_x0 & 31);  // else:
        if (node->elseBlock) {
            codegen_node_arm64(codegen, node->elseBlock);
        }
    }

    if (node->type == NODE_WHILE) {
        uint32_t *loop_label = codegen->code_word_ptr;

        uint32_t *done_label;
        if (node->condition != NULL) {
            codegen_node_arm64(codegen, node->condition);
            done_label = codegen->code_word_ptr;
            arm64_inst(0);  // cbz x0, done
        }

        codegen_node_arm64(codegen, node->thenBlock);

        arm64_inst(0x14000000 | ((loop_label - codegen->code_word_ptr) & 0x7ffffff));  // b loop

        if (node->condition != NULL) {
            *done_label = 0xB4000000 | (((codegen->code_word_ptr - done_label) & 0x7ffff) << 5) | (arm64_x0 & 31);  // done:
        }
    }

    if (node->type == NODE_DOWHILE) {
        uint32_t *loop_label = codegen->code_word_ptr;

        codegen_node_arm64(codegen, node->thenBlock);

        codegen_node_arm64(codegen, node->condition);
        arm64_inst(0xB5000000 | (((loop_label - codegen->code_word_ptr) & 0x7ffff) << 5) | (arm64_x0 & 31));  // cbnz x0, loop
    }

    if (node->type == NODE_RETURN) {
        codegen_node_arm64(codegen, node->unary);

        if (codegen->currentFunction->locals.filled > 0) {
            arm64_inst(0x910003BF);                    // mov sp, fp
            arm64_inst(0xF84107E0 | (arm64_fp & 31));  // ldr fp, [sp], 16
        }
        arm64_inst(0xD65F03C0);  // ret
        return;
    }

    if (node->type > NODE_UNARY_BEGIN && node->type < NODE_UNARY_END) {
        codegen_node_arm64(codegen, node->unary);

        if (node->type == NODE_NEG) arm64_inst(0xCB0003E0);  // sub x0, xzr, x0
        if (node->type == NODE_NOT) arm64_inst(0xAA2003E0);  // mvn x0, x0
        if (node->type == NODE_LOGICAL_NOT) {
            arm64_inst(0xF100001F);  // cmp x0, 0
            arm64_inst(0x9A9F17E0);  // cset x0, eq
        }
        return;
    }

    if (node->type == NODE_ASSIGN) {
        codegen_node_arm64(codegen, node->rhs);

        arm64_inst(0xD1000000 | ((node->lhs->local->address & 0x1fff) << 10) | ((arm64_fp & 31) << 5) | (arm64_x1 & 31));  // sub x1, fp, imm
        arm64_inst(0xF9000020);                                                                                            // str x0, [x1]
        return;
    }

    if (node->type > NODE_OPERATION_BEGIN && node->type < NODE_OPERATION_END) {
        codegen_node_arm64(codegen, node->rhs);
        arm64_inst(0xF81F0FE0 | (arm64_x0 & 31));  // str x0, [sp, -16]!

        codegen_node_arm64(codegen, node->lhs);
        arm64_inst(0xF84107E0 | (arm64_x1 & 31));  // ldr x1, [sp], 16

        if (node->type == NODE_ADD) arm64_inst(0x8B010000);  // add x0, x0, x1
        if (node->type == NODE_SUB) arm64_inst(0xCB010000);  // sub x0, x0, x1
        if (node->type == NODE_MUL) arm64_inst(0x9B017C00);  // mul x0, x0, x1
        if (node->type == NODE_DIV) arm64_inst(0x9AC10C00);  // sdiv x0, x0, x1
        if (node->type == NODE_MOD) {
            arm64_inst(0x9AC10802);  // udiv x2, x0, x1
            arm64_inst(0x9B018040);  // msub x0, x2, x1, x0
        }
        if (node->type == NODE_AND) arm64_inst(0x8A010000);  // and x0, x0, x1
        if (node->type == NODE_OR) arm64_inst(0xAA010000);   // orr x0, x0, x1
        if (node->type == NODE_XOR) arm64_inst(0xCA010000);  // eor x0, x0, x1
        if (node->type == NODE_SHL) arm64_inst(0x9AC12000);  // lsl x0, x0, x1
        if (node->type == NODE_SHR) arm64_inst(0x9AC12400);  // lsr x0, x0, x1

        if (node->type > NODE_COMPARE_BEGIN && node->type < NODE_COMPARE_END) {
            arm64_inst(0xEB01001F);                               // cmp x0, x1
            if (node->type == NODE_EQ) arm64_inst(0x9A9F17E0);    // cset x0, eq
            if (node->type == NODE_NEQ) arm64_inst(0x9A9F07E0);   // cset x0, ne
            if (node->type == NODE_LT) arm64_inst(0x9A9FA7E0);    // cset x0, lt
            if (node->type == NODE_LTEQ) arm64_inst(0x9A9FC7E0);  // cset x0, le
            if (node->type == NODE_GT) arm64_inst(0x9A9FD7E0);    // cset x0, gt
            if (node->type == NODE_GTEQ) arm64_inst(0x9A9FB7E0);  // cset x0, ge
        }

        if (node->type == NODE_LOGICAL_AND || node->type == NODE_LOGICAL_OR) {
            if (node->type == NODE_LOGICAL_AND) arm64_inst(0x8A010000);  // and x0, x0, x1
            if (node->type == NODE_LOGICAL_OR) arm64_inst(0xAA010000);   // orr x0, x0, x1
            arm64_inst(0xF100001F);                                      // cmp x0, 0
            arm64_inst(0x9A9F07E0);                                      // cset x0, ne
        }
        return;
    }
}
