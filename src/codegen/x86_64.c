#include <string.h>

#include "codegen.h"

#define x86_64_rax 0
#define x86_64_rcx 1
#define x86_64_rdx 2
#define x86_64_rbx 3
#define x86_64_rsp 4
#define x86_64_rbp 5
#define x86_64_rsi 6
#define x86_64_rdi 7

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

void codegen_addr_x86_64(Codegen *codegen, Node *node) {
    if (node->kind == NODE_LOCAL) {
        x86_64_inst3(0x48, 0x8d, 0x85);  // lea rax, qword [rbp - imm]
        x86_64_imm32(-node->local->offset);
        return;
    }
    if (node->kind == NODE_DEREF) {
        codegen_node_x86_64(codegen, node->lhs);
        return;
    }

    print_error(codegen->text, node->token, "Node is not an lvalue");
}

void codegen_node_x86_64(Codegen *codegen, Node *node) {
    // Program
    if (node->kind == NODE_PROGRAM) {
        codegen->program = node;
        for (size_t i = 0; i < node->nodes.size; i++) {
            Node *function_node = node->nodes.items[i];
            codegen_node_x86_64(codegen, function_node);
        }
    }

    // Function
    if (node->kind == NODE_FUNCTION) {
        codegen->current_function = node;

        // Set function ptr to current code offset
        node->function->address = codegen->code_byte_ptr;

        // Set main address
        if (!strcmp(node->function->name, "main")) {
            codegen->main = codegen->code_byte_ptr;
        }

        // Allocate locals stack frame
        size_t aligned_locals_size = align(node->locals_size, 16);
        if (aligned_locals_size > 0) {
            x86_64_inst1(0x50 | (x86_64_rbp & 7));  // push rbp
            x86_64_inst3(0x48, 0x89, 0xe5);         // mov rbp, rsp
            x86_64_inst3(0x48, 0x81, 0xec);         // sub rsp, imm
            x86_64_imm32(aligned_locals_size);
        }

        // Write arguments to locals
        for (int32_t i = node->function->arguments.size - 1; i >= 0; i--) {
            Argument *argument = node->function->arguments.items[i];
            Local *local = node_find_local(node, argument->name);
            if (i == 0) x86_64_inst3(0x48, 0x89, 0xbd);  // mov qword [rbp - imm], rdi
            if (i == 1) x86_64_inst3(0x48, 0x89, 0xb5);  // mov qword [rbp - imm], rsi
            if (i == 2) x86_64_inst3(0x48, 0x89, 0x95);  // mov qword [rbp - imm], rdx
            if (i == 3) x86_64_inst3(0x48, 0x89, 0x8d);  // mov qword [rbp - imm], rcx
            if (i == 4) x86_64_inst3(0x4c, 0x89, 0x85);  // mov qword [rbp - imm], r8
            if (i == 5) x86_64_inst3(0x4c, 0x89, 0x8d);  // mov qword [rbp - imm], r9
            x86_64_imm32(-local->offset);
        }

        for (size_t i = 0; i < node->nodes.size; i++) {
            Node *child = node->nodes.items[i];
            codegen_node_x86_64(codegen, child);
        }
        return;
    }

    // Nodes
    if (node->kind == NODE_NODES) {
        for (size_t i = 0; i < node->nodes.size; i++) {
            Node *child = node->nodes.items[i];
            codegen_node_x86_64(codegen, child);
        }
        return;
    }

    // Statements
    if (node->kind == NODE_TENARY || node->kind == NODE_IF) {
        codegen_node_x86_64(codegen, node->condition);
        x86_64_inst4(0x48, 0x83, 0xf8, 0x00);  // cmp rax, 0
        x86_64_inst2(0x0f, 0x84);              // je else
        uint8_t *else_label = codegen->code_byte_ptr;
        x86_64_imm32(0);

        codegen_node_x86_64(codegen, node->then_block);

        uint8_t *done_label;
        if (node->else_block) {
            x86_64_inst1(0xe9);  // jmp done
            done_label = codegen->code_byte_ptr;
            x86_64_imm32(0);
        }

        *((int32_t *)else_label) = codegen->code_byte_ptr - (else_label + sizeof(int32_t));  // else:
        if (node->else_block) {
            codegen_node_x86_64(codegen, node->else_block);

            *((int32_t *)done_label) = codegen->code_byte_ptr - (done_label + sizeof(int32_t));  // done:
        }
    }

    if (node->kind == NODE_WHILE) {
        uint8_t *loop_label = codegen->code_byte_ptr;

        uint8_t *done_label;
        if (node->condition != NULL) {
            codegen_node_x86_64(codegen, node->condition);
            x86_64_inst4(0x48, 0x83, 0xf8, 0x00);  // cmp rax, 0
            x86_64_inst2(0x0f, 0x84);              // je done
            done_label = codegen->code_byte_ptr;
            x86_64_imm32(0);
        }

        codegen_node_x86_64(codegen, node->then_block);

        x86_64_inst1(0xe9);  // jmp loop
        x86_64_imm32(loop_label - (codegen->code_byte_ptr + sizeof(int32_t)));

        if (node->condition != NULL) {
            *((int32_t *)done_label) = codegen->code_byte_ptr - (done_label + sizeof(int32_t));  // done:
        }
    }

    if (node->kind == NODE_DOWHILE) {
        uint8_t *loop_label = codegen->code_byte_ptr;

        codegen_node_x86_64(codegen, node->then_block);

        codegen_node_x86_64(codegen, node->condition);
        x86_64_inst4(0x48, 0x83, 0xf8, 0x00);  // cmp rax, 0
        x86_64_inst2(0x0f, 0x85);              // jne loop
        x86_64_imm32(loop_label - (codegen->code_byte_ptr + sizeof(int32_t)));
    }

    if (node->kind == NODE_RETURN) {
        codegen_node_x86_64(codegen, node->unary);

        // Free locals stack frame
        if (codegen->current_function->locals_size > 0) {
            x86_64_inst3(0x48, 0x89, 0xec);         // mov rsp, rbp
            x86_64_inst1(0x58 | (x86_64_rbp & 7));  // pop rbp
        }
        x86_64_inst1(0xc3);  // ret
        return;
    }

    // Unary
    if (node->kind == NODE_ADDR) {
        codegen_addr_x86_64(codegen, node->unary);
        return;
    }

    if (node->kind == NODE_DEREF) {
        codegen_node_x86_64(codegen, node->unary);

        // When array in array just return the pointer
        if (node->type->kind != TYPE_ARRAY) {
            x86_64_inst3(0x48, 0x8b, 0x00);  // mov rax, qword [rax]
        }
        return;
    }

    if (node->kind > NODE_UNARY_BEGIN && node->kind < NODE_UNARY_END) {
        codegen_node_x86_64(codegen, node->unary);

        if (node->kind == NODE_NEG) x86_64_inst3(0x48, 0xf7, 0xd8);  // neg rax
        if (node->kind == NODE_NOT) x86_64_inst3(0x48, 0xf7, 0xd0);  // not rax
        if (node->kind == NODE_LOGICAL_NOT) {
            x86_64_inst4(0x48, 0x83, 0xf8, 0x00);  // cmp rax, 0
            x86_64_inst3(0x0f, 0x94, 0xc0);        // sete al
            x86_64_inst4(0x48, 0x0f, 0xb6, 0xc0);  // movzx rax, al
        }
        return;
    }

    // Operators
    if (node->kind == NODE_ASSIGN) {
        codegen_addr_x86_64(codegen, node->lhs);
        x86_64_inst1(0x50 | (x86_64_rax & 7));  // push rax

        codegen_node_x86_64(codegen, node->rhs);
        x86_64_inst1(0x58 | (x86_64_rcx & 7));  // pop rcx

        x86_64_inst3(0x48, 0x89, 0x01);  // mov qword [rcx], rax
        return;
    }

    if (node->kind > NODE_OPERATION_BEGIN && node->kind < NODE_OPERATION_END) {
        codegen_node_x86_64(codegen, node->rhs);
        x86_64_inst1(0x50 | (x86_64_rax & 7));  // push rax

        codegen_node_x86_64(codegen, node->lhs);
        x86_64_inst1(0x58 | (x86_64_rcx & 7));  // pop rcx

        if (node->kind == NODE_ADD) x86_64_inst3(0x48, 0x01, 0xc8);        // add rax, rcx
        if (node->kind == NODE_SUB) x86_64_inst3(0x48, 0x29, 0xc8);        // sub rax, rcx
        if (node->kind == NODE_MUL) x86_64_inst4(0x48, 0x0f, 0xaf, 0xc1);  // imul rax, rcx
        if (node->kind == NODE_DIV) {
            x86_64_inst2(0x48, 0x99);        // cqo
            x86_64_inst3(0x48, 0xf7, 0xf9);  // idiv rcx
        }
        if (node->kind == NODE_MOD) {
            x86_64_inst2(0x48, 0x99);        // cqo
            x86_64_inst3(0x48, 0xf7, 0xf9);  // idiv rcx
            x86_64_inst3(0x48, 0x89, 0xd0);  // mov rax, rdx
        }
        if (node->kind == NODE_AND) x86_64_inst3(0x48, 0x21, 0xc8);  // and rax, rcx
        if (node->kind == NODE_OR) x86_64_inst3(0x48, 0x09, 0xc8);   // or rax, rcx
        if (node->kind == NODE_XOR) x86_64_inst3(0x48, 0x31, 0xc8);  // xor rax, rcx
        if (node->kind == NODE_SHL) x86_64_inst3(0x48, 0xd3, 0xe0);  // shl rax, cl
        if (node->kind == NODE_SHR) x86_64_inst3(0x48, 0xd3, 0xe8);  // shr rax, cl

        if (node->kind > NODE_COMPARE_BEGIN && node->kind < NODE_COMPARE_END) {
            x86_64_inst3(0x48, 0x39, 0xc8);                               // cmp rax, rcx
            if (node->kind == NODE_EQ) x86_64_inst3(0x0f, 0x94, 0xc0);    // sete al
            if (node->kind == NODE_NEQ) x86_64_inst3(0x0f, 0x95, 0xc0);   // setne al
            if (node->kind == NODE_LT) x86_64_inst3(0x0f, 0x9c, 0xc0);    // setl al
            if (node->kind == NODE_LTEQ) x86_64_inst3(0x0f, 0x9e, 0xc0);  // setle al
            if (node->kind == NODE_GT) x86_64_inst3(0x0f, 0x9f, 0xc0);    // setg al
            if (node->kind == NODE_GTEQ) x86_64_inst3(0x0f, 0x9d, 0xc0);  // setge al
            x86_64_inst4(0x48, 0x0f, 0xb6, 0xc0);                         // movzx rax, al
        }

        if (node->kind == NODE_LOGICAL_AND || node->kind == NODE_LOGICAL_OR) {
            if (node->kind == NODE_LOGICAL_AND) x86_64_inst3(0x48, 0x21, 0xc8);  // and rax, rcx
            if (node->kind == NODE_LOGICAL_OR) x86_64_inst3(0x48, 0x09, 0xc8);   // or rax, rcx
            x86_64_inst4(0x48, 0x83, 0xf8, 0x00);                                // cmp rax, 0
            x86_64_inst3(0x0f, 0x95, 0xc0);                                      // setne al
            x86_64_inst4(0x48, 0x0f, 0xb6, 0xc0);                                // movzx rax, al
        }
        return;
    }

    // Values
    if (node->kind == NODE_LOCAL) {
        // When we load an array we don't load value because the array becomes a pointer
        if (node->local->type->kind == TYPE_ARRAY) {
            codegen_addr_x86_64(codegen, node);
        } else {
            x86_64_inst3(0x48, 0x8b, 0x85);  // mov rax, qword [rbp - imm]
            x86_64_imm32(-node->local->offset);
        }
        return;
    }

    if (node->kind == NODE_INTEGER) {
        if (node->integer > INT32_MIN && node->integer < INT32_MAX) {
            x86_64_inst1(0xb8 | (x86_64_rax & 7));  // mov eax, imm
            x86_64_imm32(node->integer);
        } else {
            x86_64_inst2(0x48, 0xb8 | (x86_64_rax & 7));  // movabs rax, imm
            x86_64_imm64(node->integer);
        }
        return;
    }

    if (node->kind == NODE_CALL) {
        // Write arguments to registers
        for (int32_t i = node->nodes.size - 1; i >= 0; i--) {
            Node *argument = node->nodes.items[i];
            codegen_node_x86_64(codegen, argument);

            if (i == 0) x86_64_inst3(0x48, 0x89, 0xc7);  // mov rdi, rax
            if (i == 1) x86_64_inst3(0x48, 0x89, 0xc6);  // mov rsi, rax
            if (i == 2) x86_64_inst3(0x48, 0x89, 0xc2);  // mov rdx, rax
            if (i == 3) x86_64_inst3(0x48, 0x89, 0xc1);  // mov rcx, rax
            if (i == 4) x86_64_inst3(0x4c, 0x89, 0xc0);  // mov r8, rax
            if (i == 5) x86_64_inst3(0x4c, 0x89, 0xc1);  // mov r9, rax
        }

        x86_64_inst1(0xe8);  // call function
        x86_64_imm32((uint8_t *)node->function->address - (codegen->code_byte_ptr + sizeof(int32_t)));
        return;
    }
}
