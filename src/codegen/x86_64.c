#include <string.h>

#include "codegen.h"

#define rax 0
#define rcx 1
#define rdx 2
#define rbx 3
#define rsp 4
#define rbp 5
#define rsi 6
#define rdi 7

#define inst1(a) *codegen->code_byte_ptr++ = a
#define inst2(a, b)                      \
    {                                    \
        inst1(a);                        \
        *codegen->code_byte_ptr++ = b; \
    }
#define inst3(a, b, c)                   \
    {                                    \
        inst2(a, b);                     \
        *codegen->code_byte_ptr++ = c; \
    }
#define inst4(a, b, c, d)                \
    {                                    \
        inst3(a, b, c);                  \
        *codegen->code_byte_ptr++ = d; \
    }

#define imm32(imm)                                  \
    {                                               \
        *((int32_t *)codegen->code_byte_ptr) = imm; \
        codegen->code_byte_ptr += sizeof(int32_t);  \
    }
#define imm64(imm)                                  \
    {                                               \
        *((int64_t *)codegen->code_byte_ptr) = imm; \
        codegen->code_byte_ptr += sizeof(int64_t);  \
    }

void codegen_addr_x86_64(Codegen *codegen, Node *node) {
    if (node->kind == NODE_LOCAL) {
        inst3(0x48, 0x8d, 0x85);  // lea rax, qword [rbp - imm]
        imm32(-node->local->offset);
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
            inst1(0x50 | (rbp & 7));  // push rbp
            inst3(0x48, 0x89, 0xe5);  // mov rbp, rsp
            inst3(0x48, 0x81, 0xec);  // sub rsp, imm
            imm32(aligned_locals_size);
        }

        // Write arguments to locals
        for (int32_t i = node->function->arguments.size - 1; i >= 0; i--) {
            Argument *argument = node->function->arguments.items[i];
            Local *local = node_find_local(node, argument->name);

            if (i == 0) {
                if (local->type->size == 1) inst3(0x40, 0x88, 0xbd);  // mov byte [rbp - imm], dil
                if (local->type->size == 2) inst3(0x66, 0x89, 0xbd);  // mov word [rbp - imm], di
                if (local->type->size == 4) inst2(0x89, 0xbd);        // mov dword [rbp - imm], edi
                if (local->type->size == 8) inst3(0x48, 0x89, 0xbd);  // mov qword [rbp - imm], rdi
            }
            if (i == 1) {
                if (local->type->size == 1) inst3(0x40, 0x88, 0xb5);  // mov byte [rbp - imm], sil
                if (local->type->size == 2) inst3(0x66, 0x89, 0xb5);  // mov word [rbp - imm], si
                if (local->type->size == 4) inst2(0x89, 0xb5);        // mov dword [rbp - imm], esi
                if (local->type->size == 8) inst3(0x48, 0x89, 0xb5);  // mov qword [rbp - imm], rsi
            }
            if (i == 2) {
                if (local->type->size == 1) inst2(0x88, 0x95);        // mov byte [rbp - imm], dl
                if (local->type->size == 2) inst3(0x66, 0x89, 0x95);  // mov word [rbp - imm], dx
                if (local->type->size == 4) inst2(0x89, 0x95);        // mov dword [rbp - imm], edx
                if (local->type->size == 8) inst3(0x48, 0x89, 0x95);  // mov qword [rbp - imm], rdx
            }
            if (i == 3) {
                if (local->type->size == 1) inst2(0x88, 0x8d);        // mov byte [rbp - imm], cl
                if (local->type->size == 2) inst3(0x66, 0x89, 0x8d);  // mov word [rbp - imm], cx
                if (local->type->size == 4) inst2(0x89, 0x8d);        // mov dword [rbp - imm], ecx
                if (local->type->size == 8) inst3(0x48, 0x89, 0x8d);  // mov qword [rbp - imm], rcx
            }
            imm32(-local->offset);
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
        inst4(0x48, 0x83, 0xf8, 0x00);  // cmp rax, 0
        inst2(0x0f, 0x84);              // je else
        uint8_t *else_label = codegen->code_byte_ptr;
        imm32(0);

        codegen_node_x86_64(codegen, node->then_block);

        uint8_t *done_label;
        if (node->else_block) {
            inst1(0xe9);  // jmp done
            done_label = codegen->code_byte_ptr;
            imm32(0);
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
            inst4(0x48, 0x83, 0xf8, 0x00);  // cmp rax, 0
            inst2(0x0f, 0x84);              // je done
            done_label = codegen->code_byte_ptr;
            imm32(0);
        }

        codegen_node_x86_64(codegen, node->then_block);

        inst1(0xe9);  // jmp loop
        imm32(loop_label - (codegen->code_byte_ptr + sizeof(int32_t)));

        if (node->condition != NULL) {
            *((int32_t *)done_label) = codegen->code_byte_ptr - (done_label + sizeof(int32_t));  // done:
        }
    }

    if (node->kind == NODE_DOWHILE) {
        uint8_t *loop_label = codegen->code_byte_ptr;

        codegen_node_x86_64(codegen, node->then_block);

        codegen_node_x86_64(codegen, node->condition);
        inst4(0x48, 0x83, 0xf8, 0x00);  // cmp rax, 0
        inst2(0x0f, 0x85);              // jne loop
        imm32(loop_label - (codegen->code_byte_ptr + sizeof(int32_t)));
    }

    if (node->kind == NODE_RETURN) {
        codegen_node_x86_64(codegen, node->unary);

        // Free locals stack frame
        if (codegen->current_function->locals_size > 0) {
            inst3(0x48, 0x89, 0xec);  // mov rsp, rbp
            inst1(0x58 | (rbp & 7));  // pop rbp
        }
        inst1(0xc3);  // ret
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
            inst3(0x48, 0x8b, 0x00);  // mov rax, qword [rax]
        }
        return;
    }

    if (node->kind > NODE_UNARY_BEGIN && node->kind < NODE_UNARY_END) {
        codegen_node_x86_64(codegen, node->unary);

        if (node->kind == NODE_NEG) inst3(0x48, 0xf7, 0xd8);  // neg rax
        if (node->kind == NODE_NOT) inst3(0x48, 0xf7, 0xd0);  // not rax
        if (node->kind == NODE_LOGICAL_NOT) {
            inst4(0x48, 0x83, 0xf8, 0x00);  // cmp rax, 0
            inst3(0x0f, 0x94, 0xc0);        // sete al
            inst4(0x48, 0x0f, 0xb6, 0xc0);  // movzx rax, al
        }
        return;
    }

    // Operators
    if (node->kind == NODE_ASSIGN) {
        codegen_addr_x86_64(codegen, node->lhs);
        inst1(0x50 | (rax & 7));  // push rax

        codegen_node_x86_64(codegen, node->rhs);
        inst1(0x58 | (rcx & 7));  // pop rcx

        Type *type = node->lhs->type;
        if (type->size == 1) inst3(0x88, 0x41, 0x01);  // mov byte [rcx], al
        if (type->size == 2) inst3(0x66, 0x89, 0x01);  // mov word [rcx], ax
        if (type->size == 4) inst2(0x89, 0x01);        // mov dword [rcx], eax
        if (type->size == 8) inst3(0x48, 0x89, 0x01);  // mov qword [rcx], rax
        return;
    }

    if (node->kind > NODE_OPERATION_BEGIN && node->kind < NODE_OPERATION_END) {
        codegen_node_x86_64(codegen, node->rhs);
        inst1(0x50 | (rax & 7));  // push rax

        codegen_node_x86_64(codegen, node->lhs);
        inst1(0x58 | (rcx & 7));  // pop rcx

        if (node->kind == NODE_ADD) inst3(0x48, 0x01, 0xc8);        // add rax, rcx
        if (node->kind == NODE_SUB) inst3(0x48, 0x29, 0xc8);        // sub rax, rcx
        if (node->kind == NODE_MUL) inst4(0x48, 0x0f, 0xaf, 0xc1);  // imul rax, rcx
        if (node->kind == NODE_DIV) {
            inst2(0x48, 0x99);        // cqo
            inst3(0x48, 0xf7, 0xf9);  // idiv rcx
        }
        if (node->kind == NODE_MOD) {
            inst2(0x48, 0x99);        // cqo
            inst3(0x48, 0xf7, 0xf9);  // idiv rcx
            inst3(0x48, 0x89, 0xd0);  // mov rax, rdx
        }
        if (node->kind == NODE_AND) inst3(0x48, 0x21, 0xc8);  // and rax, rcx
        if (node->kind == NODE_OR) inst3(0x48, 0x09, 0xc8);   // or rax, rcx
        if (node->kind == NODE_XOR) inst3(0x48, 0x31, 0xc8);  // xor rax, rcx
        if (node->kind == NODE_SHL) inst3(0x48, 0xd3, 0xe0);  // shl rax, cl
        if (node->kind == NODE_SHR) inst3(0x48, 0xd3, 0xe8);  // shr rax, cl

        if (node->kind > NODE_COMPARE_BEGIN && node->kind < NODE_COMPARE_END) {
            inst3(0x48, 0x39, 0xc8);                               // cmp rax, rcx
            if (node->kind == NODE_EQ) inst3(0x0f, 0x94, 0xc0);    // sete al
            if (node->kind == NODE_NEQ) inst3(0x0f, 0x95, 0xc0);   // setne al
            if (node->kind == NODE_LT) inst3(0x0f, 0x9c, 0xc0);    // setl al
            if (node->kind == NODE_LTEQ) inst3(0x0f, 0x9e, 0xc0);  // setle al
            if (node->kind == NODE_GT) inst3(0x0f, 0x9f, 0xc0);    // setg al
            if (node->kind == NODE_GTEQ) inst3(0x0f, 0x9d, 0xc0);  // setge al
            inst4(0x48, 0x0f, 0xb6, 0xc0);                         // movzx rax, al
        }

        if (node->kind == NODE_LOGICAL_AND || node->kind == NODE_LOGICAL_OR) {
            if (node->kind == NODE_LOGICAL_AND) inst3(0x48, 0x21, 0xc8);  // and rax, rcx
            if (node->kind == NODE_LOGICAL_OR) inst3(0x48, 0x09, 0xc8);   // or rax, rcx
            inst4(0x48, 0x83, 0xf8, 0x00);                                // cmp rax, 0
            inst3(0x0f, 0x95, 0xc0);                                      // setne al
            inst4(0x48, 0x0f, 0xb6, 0xc0);                                // movzx rax, al
        }
        return;
    }

    // Values
    if (node->kind == NODE_LOCAL) {
        // When we load an array we don't load value because the array becomes a pointer
        Type *type = node->local->type;
        if (type->kind == TYPE_ARRAY) {
            codegen_addr_x86_64(codegen, node);
        } else {
            if (type->size == 1) inst2(0x8a, 0x85);        // mov al, byte [rbp - imm]
            if (type->size == 2) inst3(0x66, 0x8b, 0x85);  // mov ax, word [rbp - imm]
            if (type->size == 4) inst2(0x8b, 0x85);        // mov eax, dword [rbp - imm]
            if (type->size == 8) inst3(0x48, 0x8b, 0x85);  // mov rax, qword [rbp - imm]
            imm32(-node->local->offset);
            if (type->size == 1) inst4(0x48, 0x0f, 0xb6, 0xc0);  // movzx rax, al
            if (type->size == 2) inst4(0x48, 0x0f, 0xb7, 0xc0);  // movzx rax, ax
        }
        return;
    }

    if (node->kind == NODE_INTEGER) {
        if (node->integer > INT32_MIN && node->integer < INT32_MAX) {
            inst1(0xb8 | (rax & 7));  // mov eax, imm
            imm32(node->integer);
        } else {
            inst2(0x48, 0xb8 | (rax & 7));  // movabs rax, imm
            imm64(node->integer);
        }
        return;
    }

    if (node->kind == NODE_CALL) {
        // Write arguments to registers
        for (int32_t i = node->nodes.size - 1; i >= 0; i--) {
            Node *argument = node->nodes.items[i];
            codegen_node_x86_64(codegen, argument);

            if (i == 0) inst3(0x48, 0x89, 0xc7);  // mov rdi, rax
            if (i == 1) inst3(0x48, 0x89, 0xc6);  // mov rsi, rax
            if (i == 2) inst3(0x48, 0x89, 0xc2);  // mov rdx, rax
            if (i == 3) inst3(0x48, 0x89, 0xc1);  // mov rcx, rax
        }

        inst1(0xe8);  // call function
        imm32((uint8_t *)node->function->address - (codegen->code_byte_ptr + sizeof(int32_t)));
        return;
    }
}
