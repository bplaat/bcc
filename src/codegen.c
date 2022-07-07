#include "codegen.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "type.h"

Arch *arch;
Node *currentBlock;
int32_t depth = 0;
int32_t unique = 1;
int32_t returnId;
bool inAssign = false;

void node_asm(FILE *file, Node *node, Node *next) {
    if (node->kind == NODE_MULTIPLE) {
        for (size_t i = 0; i < node->nodes->size; i++) {
            Node *statement = list_get(node->nodes, i);
            Node *next = i != node->nodes->size - 1 ? list_get(node->nodes, i + 1) : NULL;
            if (statement->kind != NODE_NULL) {
                node_asm(file, statement, next);
                fprintf(file, "\n");
            }
        }
    }

    if (node->kind == NODE_FUNCTION) {
        fprintf(file, "_%s:\n", node->name);
        returnId = unique++;
        node_asm(file, node->block, NULL);
        fprintf(file, "\n");
    }

    if (node->kind == NODE_BLOCK) {
        size_t stackSize = 0;
        for (size_t i = 0; i < node->locals->size; i++) {
            Local *local = list_get(node->locals, i);
            stackSize += align(local->type->size, arch->stackAlign);
        }
        if (stackSize > 0) {
            if (arch->kind == ARCH_ARM64) {
                if (depth > 0) fprintf(file, "    str x29, [sp, -%d]!\n", arch->stackAlign);
                fprintf(file, "    mov x29, sp\n");
                fprintf(file, "    sub sp, sp, %zu\n\n", stackSize);
            }
            if (arch->kind == ARCH_X86_64) {
                if (depth > 0) fprintf(file, "    push rbp\n");
                fprintf(file, "    mov rbp, rsp\n");
                fprintf(file, "    sub rsp, %zu\n\n", stackSize);
            }
        }

        depth++;
        for (size_t i = 0; i < node->nodes->size; i++) {
            Node *statement = list_get(node->nodes, i);
            Node *next = i != node->nodes->size - 1 ? list_get(node->nodes, i + 1) : NULL;
            currentBlock = node;
            if (statement->kind != NODE_NULL) {
                node_asm(file, statement, next);
                fprintf(file, "\n");
            }
        }
        depth--;

        if (depth == 0) {
            fprintf(file, ".b%d:\n", returnId);
        }
        if (stackSize > 0) {
            if (arch->kind == ARCH_ARM64) {
                fprintf(file, "    mov sp, x8\n");
                if (depth > 0) fprintf(file, "    ldr x29, [sp], %d\n", arch->stackAlign);
            }
            if (arch->kind == ARCH_X86_64) {
                if (depth > 0) {
                    fprintf(file, "    leave\n");
                } else {
                    fprintf(file, "    mov rsp, rbp\n");
                }
            }
        }
        if (depth == 0) {
            fprintf(file, "    ret\n");
        }
    }

    if (node->kind == NODE_IF) {
        node_asm(file, node->condition, NULL);
        int32_t endId = unique++;
        if (arch->kind == ARCH_ARM64) {
            if (node->elseBlock != NULL) {
                int32_t elseId = unique++;
                fprintf(file, "    cbz x0, .b%d\n", elseId);
                node_asm(file, node->thenBlock, NULL);
                fprintf(file, "    b .b%d\n", endId);
                fprintf(file, ".b%d:\n", elseId);
                node_asm(file, node->elseBlock, NULL);
                fprintf(file, ".b%d:\n", endId);
            } else {
                fprintf(file, "    cbz x0, .b%d\n", endId);
                node_asm(file, node->thenBlock, NULL);
                fprintf(file, ".b%d:\n", endId);
            }
        }
        if (arch->kind == ARCH_X86_64) {
            if (node->elseBlock != NULL) {
                int32_t elseId = unique++;
                fprintf(file, "    cmp rax, 0\n");
                fprintf(file, "    je .b%d\n", elseId);
                node_asm(file, node->thenBlock, NULL);
                fprintf(file, "    jmp .b%d\n", endId);
                fprintf(file, ".b%d:\n", elseId);
                node_asm(file, node->elseBlock, NULL);
                fprintf(file, ".b%d:\n", endId);
            } else {
                fprintf(file, "    cmp rax, 0\n");
                fprintf(file, "    je .b%d\n", endId);
                node_asm(file, node->thenBlock, NULL);
                fprintf(file, ".b%d:\n", endId);
            }
        }
    }

    if (node->kind == NODE_WHILE) {
        int32_t beginId = unique++;
        fprintf(file, ".b%d:\n", beginId);
        if (node->condition != NULL) node_asm(file, node->condition, NULL);
        int32_t endId = unique++;
        if (arch->kind == ARCH_ARM64) {
            if (node->condition != NULL) fprintf(file, "    cbz x0, .b%d\n", endId);
            node_asm(file, node->thenBlock, NULL);
            fprintf(file, "    b .b%d\n", beginId);
            fprintf(file, ".b%d:\n", endId);
        }
        if (arch->kind == ARCH_X86_64) {
            if (node->condition != NULL) {
                fprintf(file, "    cmp rax, 0\n");
                fprintf(file, "    je .b%d\n", endId);
            }
            node_asm(file, node->thenBlock, NULL);
            fprintf(file, "    jmp .b%d\n", beginId);
            fprintf(file, ".b%d:\n", endId);
        }
    }

    if (node->kind == NODE_RETURN) {
        node_asm(file, node->unary, NULL);
        if (!(depth == 1 && next == NULL)) {
            if (arch->kind == ARCH_ARM64) fprintf(file, "    b .b%d\n", returnId);
            if (arch->kind == ARCH_X86_64) fprintf(file, "    jmp .b%d\n", returnId);
        }
    }

    if (node->kind == NODE_NUMBER) {
        fprintf(file, "    ; ");
        node_print(file, node);
        fprintf(file, "\n");

        if (arch->kind == ARCH_ARM64) {
            fprintf(file, "    mov w0, %d\n", (int32_t)node->number & 0xffff);
            if (node->number >= 0xffff) {
                fprintf(file, "    movk w0, %d, lsl 16\n", ((int32_t)node->number >> 16) & 0xffff);
            }
        }
        if (arch->kind == ARCH_X86_64) {
            fprintf(file, "    mov eax, %lld\n", node->number);
        }
    }

    if (node->kind == NODE_VARIABLE) {
        fprintf(file, "    ; ");
        node_print(file, node);
        fprintf(file, "\n");

        Local *local = node_find_local(currentBlock, node->string);
        if (inAssign) {
            if (arch->kind == ARCH_ARM64) fprintf(file, "    sub x0, x29, %zu\n", local->offset);
            if (arch->kind == ARCH_X86_64) fprintf(file, "    lea rax, [rbp - %zu]\n", local->offset);
        } else {
            if (arch->kind == ARCH_ARM64) fprintf(file, "    ldr %c0, [x29, -%zu]\n", node->type->size == 8 ? 'x' : 'w', local->offset);
            if (arch->kind == ARCH_X86_64) fprintf(file, "    mov %cax, [rbp - %zu]\n", node->type->size == 8 ? 'r' : 'e', local->offset);
        }
    }

    if (node->kind == NODE_FUNCCALL) {
        fprintf(file, "    ; ");
        node_print(file, node);
        fprintf(file, "\n");

        for (size_t i = 0; i < node->nodes->size; i++) {
            Node *statement = list_get(node->nodes, i);
            if (statement->kind != NODE_NULL) {
                node_asm(file, statement, NULL);
                fprintf(file, "\n");
                if (arch->kind == ARCH_ARM64) fprintf(file, "    str x0, [sp, -%d]!\n", arch->stackAlign);
                if (arch->kind == ARCH_X86_64) fprintf(file, "    push rax\n");
            }
        }

        if (arch->kind == ARCH_ARM64) {
            for (int32_t i = (int32_t)node->nodes->size - 1; i >= 0; i--) {
                fprintf(file, "    ldr x%d, [sp], %d\n", i, arch->stackAlign);
            }
            fprintf(file, "    str x30, [sp, -%d]!\n", arch->stackAlign);
            fprintf(file, "    bl _%s\n", node->string);
            fprintf(file, "    ldr x30, [sp], %d\n", arch->stackAlign);
        }
        if (arch->kind == ARCH_X86_64) {
            char *argregs[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
            for (int32_t i = (int32_t)node->nodes->size - 1; i >= 0; i--) {
                fprintf(file, "    pop %s\n", argregs[i]);
            }
            fprintf(file, "    xor rax, rax\n");
            fprintf(file, "    extern _%s\n", node->string);
            fprintf(file, "    call _%s\n", node->string);
        }
    }

    if (node->kind >= NODE_NEG && node->kind <= NODE_LOGIC_NOT) {
        fprintf(file, "    ; ");
        node_print(file, node);
        fprintf(file, "\n");

        if (node->kind == NODE_ADDR) {
            Local *local = node_find_local(currentBlock, node->unary->string);
            if (arch->kind == ARCH_ARM64) fprintf(file, "    sub x0, x29, %zu\n", local->offset);
            if (arch->kind == ARCH_X86_64) fprintf(file, "    lea rax, [rbp - %zu]\n", local->offset);
            return;
        }

        node_asm(file, node->unary, NULL);

        if (node->kind == NODE_NEG) {
            if (arch->kind == ARCH_ARM64) fprintf(file, "    sub x0, xzr, x0\n");
            if (arch->kind == ARCH_X86_64) fprintf(file, "    neg rax\n");
        }

        if (node->kind == NODE_DEREF && (!inAssign || node->unary->kind == NODE_VARIABLE)) {
            if (arch->kind == ARCH_ARM64) fprintf(file, "    ldr %c0, [x0]\n", node->type->size == 8 ? 'x' : 'w');
            if (arch->kind == ARCH_X86_64) fprintf(file, "    mov %cax, [rax]\n", node->type->size == 8 ? 'r' : 'e');
        }

        if (node->kind == NODE_LOGIC_NOT) {
            if (arch->kind == ARCH_ARM64) {
                fprintf(file, "    cmp x0, 0\n");
                fprintf(file, "    cset x0, eq\n");
            }
            if (arch->kind == ARCH_X86_64) {
                fprintf(file, "    cmp rax, 0\n");
                fprintf(file, "    sete al\n");
                fprintf(file, "    movzx rax, al\n");
            }
        }
    }

    if (node->kind >= NODE_ASSIGN && node->kind <= NODE_LOGIC_OR) {
        node_asm(file, node->rhs, NULL);

        if (!(node->kind == NODE_ASSIGN && node->rhs->kind == NODE_ASSIGN)) {
            if (arch->kind == ARCH_ARM64) fprintf(file, "    str x0, [sp, -%d]!\n", arch->stackAlign);
            if (arch->kind == ARCH_X86_64) fprintf(file, "    push rax\n");
        }

        if (node->kind == NODE_ASSIGN) inAssign = true;
        node_asm(file, node->lhs, NULL);
        if (node->kind == NODE_ASSIGN) inAssign = false;
        if (!(node->kind == NODE_ASSIGN && node->rhs->kind == NODE_ASSIGN)) {
            if (arch->kind == ARCH_ARM64) fprintf(file, "    ldr x1, [sp], %d\n", arch->stackAlign);
            if (arch->kind == ARCH_X86_64) fprintf(file, "    pop rdx\n");
        }

        fprintf(file, "    ; ");
        node_print(file, node);
        fprintf(file, "\n");

        if (node->kind == NODE_ASSIGN) {
            if (arch->kind == ARCH_ARM64) fprintf(file, "    str %c1, [x0]\n", node->lhs->type->size == 8 ? 'x' : 'w');
            if (arch->kind == ARCH_X86_64) fprintf(file, "    mov [rax], %cdx\n", node->lhs->type->size == 8 ? 'r' : 'e');
        }
        if (node->kind == NODE_ADD) {
            if (arch->kind == ARCH_ARM64) fprintf(file, "    add x0, x0, x1\n");
            if (arch->kind == ARCH_X86_64) fprintf(file, "    add rax, rdx\n");
        }
        if (node->kind == NODE_SUB) {
            if (arch->kind == ARCH_ARM64) fprintf(file, "    sub x0, x0, x1\n");
            if (arch->kind == ARCH_X86_64) fprintf(file, "    sub rax, rdx\n");
        }
        if (node->kind == NODE_MUL) {
            if (arch->kind == ARCH_ARM64) fprintf(file, "    mul x0, x0, x1\n");
            if (arch->kind == ARCH_X86_64) fprintf(file, "    imul rdx\n");
        }
        if (node->kind == NODE_DIV) {
            if (arch->kind == ARCH_ARM64) {
                fprintf(file, "    sdiv x0, x0, x1\n");
            }
            if (arch->kind == ARCH_X86_64) {
                fprintf(file, "    mov rcx, rdx\n");
                fprintf(file, "    xor rdx, rdx\n");
                fprintf(file, "    idiv rcx\n");
            }
        }
        if (node->kind == NODE_MOD) {
            if (arch->kind == ARCH_ARM64) {
                fprintf(file, "    udiv x2, x0, x1\n");
                fprintf(file, "    msub x0, x2, x1, x0\n");
            }
            if (arch->kind == ARCH_X86_64) {
                fprintf(file, "    mov rcx, rdx\n");
                fprintf(file, "    xor rdx, rdx\n");
                fprintf(file, "    idiv rcx\n");
                fprintf(file, "    mov rax, rdx\n");
            }
        }

        if (node->kind >= NODE_EQ && node->kind <= NODE_GTEQ) {
            if (arch->kind == ARCH_ARM64) fprintf(file, "    cmp x0, x1\n");
            if (arch->kind == ARCH_X86_64) fprintf(file, "    cmp rax, rdx\n");

            if (node->kind == NODE_EQ) {
                if (arch->kind == ARCH_ARM64) fprintf(file, "    cset x0, eq\n");
                if (arch->kind == ARCH_X86_64) fprintf(file, "    sete al\n");
            }
            if (node->kind == NODE_NEQ) {
                if (arch->kind == ARCH_ARM64) fprintf(file, "    cset x0, ne\n");
                if (arch->kind == ARCH_X86_64) fprintf(file, "    setne al\n");
            }
            if (node->kind == NODE_LT) {
                if (arch->kind == ARCH_ARM64) fprintf(file, "    cset x0, lt\n");
                if (arch->kind == ARCH_X86_64) fprintf(file, "    setl al\n");
            }
            if (node->kind == NODE_LTEQ) {
                if (arch->kind == ARCH_ARM64) fprintf(file, "    cset x0, le\n");
                if (arch->kind == ARCH_X86_64) fprintf(file, "    setle al\n");
            }
            if (node->kind == NODE_GT) {
                if (arch->kind == ARCH_ARM64) fprintf(file, "    cset x0, gt\n");
                if (arch->kind == ARCH_X86_64) fprintf(file, "    setg al\n");
            }
            if (node->kind == NODE_GTEQ) {
                if (arch->kind == ARCH_ARM64) fprintf(file, "    cset x0, ge\n");
                if (arch->kind == ARCH_X86_64) fprintf(file, "    setge al\n");
            }

            if (arch->kind == ARCH_X86_64) fprintf(file, "   movzx rax, al\n");
        }

        if (node->kind == NODE_LOGIC_AND) {
            if (arch->kind == ARCH_ARM64) fprintf(file, "    and x0, x0, x1\n");
            if (arch->kind == ARCH_X86_64) fprintf(file, "    and rax, rdx\n");
        }
        if (node->kind == NODE_LOGIC_OR) {
            if (arch->kind == ARCH_ARM64) fprintf(file, "    orr x0, x0, x1\n");
            if (arch->kind == ARCH_X86_64) fprintf(file, "    or rax, rdx\n");
        }
    }
}

void codegen(FILE *file, Node *node, Arch *_arch) {
    arch = _arch;
    node_asm(file, node, NULL);
}
