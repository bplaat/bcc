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
#define x86_64_rdx 2

#define x86_64_inst1(codegen, a) *(codegen)->byte++ = a
#define x86_64_inst2(codegen, a, b) \
    {                               \
        x86_64_inst1(codegen, a);   \
        *(codegen)->byte++ = b;     \
    }
#define x86_64_inst3(codegen, a, b, c) {\
    x86_64_inst2(codegen, a, b);       \
    *(codegen)->byte++ = c; }
#define x86_64_inst4(codegen, a, b, c, d) { \
    x86_64_inst3(codegen, a, b, c);       \
    *(codegen)->byte++ = d; }

void codegen_node_x86_64(Codegen *c, Node *node) {
    if (node->type == NODE_INTEGER) {
        x86_64_inst2(c, 0x48, 0xb8 | (x86_64_rax & 7));
        *((int64_t *)c->byte) = node->integer;
        c->byte += sizeof(int64_t);
    }

    if (node->type > NODE_OPERATION_BEGIN && node->type < NODE_OPERATION_END) {
        codegen_node_x86_64(c, node->rhs);
        x86_64_inst1(c, 0x50 | (x86_64_rax & 7));  // push rax

        codegen_node_x86_64(c, node->lhs);
        x86_64_inst1(c, 0x58 | (x86_64_rdx & 7));  // pop rdx

        if (node->type == NODE_ADD) x86_64_inst3(c, 0x48, 0x01, 0xd0);  // add rax, rdx
        if (node->type == NODE_SUB) x86_64_inst3(c, 0x48, 0x29, 0xd0);  // sub rax, rdx}
    }
}

// arm64
#define arm64_x0 0
#define arm64_x1 1

#define arm64_inst(codegen, inst) *(codegen)->word++ = inst

void codegen_node_arm64(Codegen *c, Node *node) {
    if (node->type == NODE_INTEGER) {
        arm64_inst(c, 0xD2800000 | ((node->integer & 0xffff) << 5) | (arm64_x0 & 31));  // mov x0, imm
    }

    if (node->type > NODE_OPERATION_BEGIN && node->type < NODE_OPERATION_END) {
        codegen_node_arm64(c, node->rhs);
        arm64_inst(c, 0xF81F0FE0 | (arm64_x0 & 31));  // str x0, [sp, -16]!

        codegen_node_arm64(c, node->lhs);
        arm64_inst(c, 0xF84107E0 | (arm64_x1 & 31));  // ldr x1, [sp], 16

        if (node->type == NODE_ADD) arm64_inst(c, 0x8B010000);  // add x0, x0, x1
        if (node->type == NODE_SUB) arm64_inst(c, 0xCB010000);  // sub x0, x0, x1
    }
}
