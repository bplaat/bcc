#include <codegen.h>

void codegen(Arch arch, void *code, Node *node) {
    Codegen codegen = {
        .code = code,
        .byte = (uint8_t *)code,
        .word = (uint32_t *)code,
    };

    if (arch == ARCH_X86_64) {
        codegen_node_x86_64(&codegen, node);
        *codegen.byte = 0xc3;  // ret
    }

    if (arch == ARCH_ARM64) {
        codegen_node_arm64(&codegen, node);
        *codegen.word = 0xD65F03C0;  // ret
    }
}

// x86_64
#define x86_64_rax 0
#define x86_64_rcx 1
#define x86_64_rdx 2

#define x86_64_inst1(codegen, a) *(codegen)->byte++ = a
#define x86_64_inst2(codegen, a, b) \
    {                               \
        x86_64_inst1(codegen, a);   \
        *(codegen)->byte++ = b;     \
    }
#define x86_64_inst3(codegen, a, b, c) \
    {                                  \
        x86_64_inst2(codegen, a, b);   \
        *(codegen)->byte++ = c;        \
    }
#define x86_64_inst4(codegen, a, b, c, d) \
    {                                     \
        x86_64_inst3(codegen, a, b, c);   \
        *(codegen)->byte++ = d;           \
    }

void codegen_node_x86_64(Codegen *c, Node *node) {
    if (node->type == NODE_INTEGER) {
        x86_64_inst2(c, 0x48, 0xb8 | (x86_64_rax & 7)); // mov rax, imm
        *((int64_t *)c->byte) = node->integer;
        c->byte += sizeof(int64_t);
    }

    if (node->type > NODE_UNARY_BEGIN && node->type < NODE_UNARY_END) {
        codegen_node_x86_64(c, node->unary);

        if (node->type == NODE_NEG) x86_64_inst3(c, 0x48, 0xf7, 0xd8);  // neg rax
    }

    if (node->type > NODE_OPERATION_BEGIN && node->type < NODE_OPERATION_END) {
        codegen_node_x86_64(c, node->rhs);
        x86_64_inst1(c, 0x50 | (x86_64_rax & 7));  // push rax

        codegen_node_x86_64(c, node->lhs);
        if (node->type == NODE_DIV || node->type == NODE_MOD) {
            x86_64_inst1(c, 0x58 | (x86_64_rcx & 7));  // pop rcx
        } else {
            x86_64_inst1(c, 0x58 | (x86_64_rdx & 7));  // pop rdx
        }

        if (node->type == NODE_ADD) x86_64_inst3(c, 0x48, 0x01, 0xd0);        // add rax, rdx
        if (node->type == NODE_SUB) x86_64_inst3(c, 0x48, 0x29, 0xd0);        // sub rax, rdx
        if (node->type == NODE_MUL) x86_64_inst4(c, 0x48, 0x0f, 0xaf, 0xc2);  // imul rax, rdx
        if (node->type == NODE_DIV) {
            x86_64_inst2(c, 0x48, 0x99);        // cqo
            x86_64_inst3(c, 0x48, 0xf7, 0xf9);  // idiv rcx
        }
        if (node->type == NODE_MOD) {
            x86_64_inst2(c, 0x48, 0x99);        // cqo
            x86_64_inst3(c, 0x48, 0xf7, 0xf9);  // idiv rcx
            x86_64_inst3(c, 0x48, 0x89, 0xd0);  // mov rax, rdx
        }
        if (node->type > NODE_COMPARE_BEGIN && node->type < NODE_COMPARE_END) {
            x86_64_inst3(c, 0x48, 0x39, 0xd0);                               // cmp rax, rdx
            if (node->type == NODE_EQ) x86_64_inst3(c, 0x0f, 0x94, 0xc0);    // sete al
            if (node->type == NODE_NEQ) x86_64_inst3(c, 0x0f, 0x95, 0xc0);   // setne al
            if (node->type == NODE_LT) x86_64_inst3(c, 0x0f, 0x9c, 0xc0);    // setl al
            if (node->type == NODE_LTEQ) x86_64_inst3(c, 0x0f, 0x9e, 0xc0);  // setle al
            if (node->type == NODE_GT) x86_64_inst3(c, 0x0f, 0x9f, 0xc0);    // setg al
            if (node->type == NODE_GTEQ) x86_64_inst3(c, 0x0f, 0x9d, 0xc0);  // setge al
        }
    }
}

// arm64
#define arm64_x0 0
#define arm64_x1 1
#define arm64_x2 2

#define arm64_inst(codegen, inst) *(codegen)->word++ = inst

void codegen_node_arm64(Codegen *c, Node *node) {
    if (node->type == NODE_INTEGER) {
        arm64_inst(c, 0xD2800000 | ((node->integer & 0xffff) << 5) | (arm64_x0 & 31));  // mov x0, imm
    }

    if (node->type > NODE_UNARY_BEGIN && node->type < NODE_UNARY_END) {
        codegen_node_arm64(c, node->unary);

        if (node->type == NODE_NEG) arm64_inst(c, 0xCB0003E0);  // sub x0, xzr, x0
    }

    if (node->type > NODE_OPERATION_BEGIN && node->type < NODE_OPERATION_END) {
        codegen_node_arm64(c, node->rhs);
        arm64_inst(c, 0xF81F0FE0 | (arm64_x0 & 31));  // str x0, [sp, -16]!

        codegen_node_arm64(c, node->lhs);
        arm64_inst(c, 0xF84107E0 | (arm64_x1 & 31));  // ldr x1, [sp], 16

        if (node->type == NODE_ADD) arm64_inst(c, 0x8B010000);  // add x0, x0, x1
        if (node->type == NODE_SUB) arm64_inst(c, 0xCB010000);  // sub x0, x0, x1
        if (node->type == NODE_MUL) arm64_inst(c, 0x9B017C00);  // mul x0, x0, x1
        if (node->type == NODE_DIV) arm64_inst(c, 0x9AC10C00);  // sdiv x0, x0, x1
        if (node->type == NODE_MOD) {
            arm64_inst(c, 0x9AC10802);  // udiv x2, x0, x1
            arm64_inst(c, 0x9B018040);  // msub x0, x2, x1, x0
        }
        if (node->type > NODE_COMPARE_BEGIN && node->type < NODE_COMPARE_END) {
            arm64_inst(c, 0xEB01001F);                               // cmp x0, x1
            if (node->type == NODE_EQ) arm64_inst(c, 0x9A9F17E0);    // cset x0, eq
            if (node->type == NODE_NEQ) arm64_inst(c, 0x9A9F07E0);   // cset x0, ne
            if (node->type == NODE_LT) arm64_inst(c, 0x9A9FA7E0);    // cset x0, lt
            if (node->type == NODE_LTEQ) arm64_inst(c, 0x9A9FC7E0);  // cset x0, le
            if (node->type == NODE_GT) arm64_inst(c, 0x9A9FD7E0);    // cset x0, gt
            if (node->type == NODE_GTEQ) arm64_inst(c, 0x9A9FB7E0);  // cset x0, ge
        }
    }
}
