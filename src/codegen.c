#include "codegen.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "type.h"

Arch arch;
Node *currentBlock;
int32_t depth = 0;
int32_t unique = 1;
int32_t returnId;
bool inAssign = false;

void node_asm(FILE *file, Node *node) {
    if (node->kind == NODE_MULTIPLE) {
        for (size_t i = 0; i < node->nodes->size; i++) {
            Node *statement = list_get(node->nodes, i);
            if (statement->kind != NODE_NULL) {
                node_asm(file, statement);
                fprintf(file, "\n");
            }
        }
    }

    if (node->kind == NODE_FUNCTION) {
        fprintf(file, "_%s:\n", node->name);
        returnId = unique++;
        node_asm(file, node->block);
        fprintf(file, "\n");
    }

    if (node->kind == NODE_BLOCK) {
        size_t stackSize = 0;
        for (size_t i = 0; i < node->locals->size; i++) {
            Local *local = list_get(node->locals, i);
            stackSize += local->type->size;
        }
        if (stackSize > 0) {
            stackSize = align(stackSize, 16);
            if (arch == ARCH_ARM64) {
                if (depth > 0) fprintf(file, "    str x8, [sp, -16]!\n");
                fprintf(file, "    mov x8, sp\n");
                fprintf(file, "    sub sp, sp, %zu\n\n", stackSize);
            }
            if (arch == ARCH_X86_64) {
                if (depth > 0) fprintf(file, "    push rbp\n");
                fprintf(file, "    mov rbp, rsp\n");
                fprintf(file, "    sub rsp, %zu\n\n", stackSize);
            }
        }

        depth++;
        for (size_t i = 0; i < node->nodes->size; i++) {
            Node *statement = list_get(node->nodes, i);
            currentBlock = node;
            if (statement->kind != NODE_NULL) {
                node_asm(file, statement);
                fprintf(file, "\n");
            }
        }
        depth--;

        if (depth == 0) {
            fprintf(file, ".b%d:\n", returnId);
        }
        if (stackSize > 0) {
            if (arch == ARCH_ARM64) {
                fprintf(file, "    mov sp, x8\n");
                if (depth > 0) fprintf(file, "    ldr x8, [sp], 16\n");
            }
            if (arch == ARCH_X86_64) {
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
        node_asm(file, node->condition);
        int32_t endId = unique++;
        if (arch == ARCH_ARM64) {
            if (node->elseBlock != NULL) {
                int32_t elseId = unique++;
                fprintf(file, "    cbz x0, .b%d\n", elseId);
                node_asm(file, node->thenBlock);
                fprintf(file, "    b .b%d\n", endId);
                fprintf(file, ".b%d:\n", elseId);
                node_asm(file, node->elseBlock);
                fprintf(file, ".b%d:\n", endId);
            } else {
                fprintf(file, "    cbz x0, .b%d\n", endId);
                node_asm(file, node->thenBlock);
                fprintf(file, ".b%d:\n", endId);
            }
        }
        if (arch == ARCH_X86_64) {
            if (node->elseBlock != NULL) {
                int32_t elseId = unique++;
                fprintf(file, "    cmp rax, 0\n");
                fprintf(file, "    je .b%d\n", elseId);
                node_asm(file, node->thenBlock);
                fprintf(file, "    jmp .b%d\n", endId);
                fprintf(file, ".b%d:\n", elseId);
                node_asm(file, node->elseBlock);
                fprintf(file, ".b%d:\n", endId);
            } else {
                fprintf(file, "    cmp rax, 0\n");
                fprintf(file, "    je .b%d\n", endId);
                node_asm(file, node->thenBlock);
                fprintf(file, ".b%d:\n", endId);
            }
        }
    }

    if (node->kind == NODE_WHILE) {
        int32_t beginId = unique++;
        fprintf(file, ".b%d:\n", beginId);
        if (node->condition != NULL) node_asm(file, node->condition);
        int32_t endId = unique++;
        if (arch == ARCH_ARM64) {
            if (node->condition != NULL) fprintf(file, "    cbz x0, .b%d\n", endId);
            node_asm(file, node->thenBlock);
            fprintf(file, "    b .b%d\n", beginId);
            fprintf(file, ".b%d:\n", endId);
        }
        if (arch == ARCH_X86_64) {
            if (node->condition != NULL) {
                fprintf(file, "    cmp rax, 0\n");
                fprintf(file, "    je .b%d\n", endId);
            }
            node_asm(file, node->thenBlock);
            fprintf(file, "    jmp .b%d\n", beginId);
            fprintf(file, ".b%d:\n", endId);
        }
    }

    if (node->kind == NODE_RETURN) {
        node_asm(file, node->unary);
        if (arch == ARCH_ARM64) fprintf(file, "    b .b%d\n", returnId);
        if (arch == ARCH_X86_64) fprintf(file, "    jmp .b%d\n", returnId);
    }

    if (node->kind == NODE_NUMBER) {
        fprintf(file, "    ; ");
        node_print(file, node);
        fprintf(file, "\n");

        if (arch == ARCH_ARM64) {
            fprintf(file, "    mov w0, %d\n", (int32_t)node->number & 0xffff);
            if (node->number >= 0xffff) {
                fprintf(file, "    movk w0, %d, lsl 16\n", ((int32_t)node->number >> 16) & 0xffff);
            }
        }
        if (arch == ARCH_X86_64) {
            fprintf(file, "    mov rax, %lld\n", node->number);
        }
    }

    if (node->kind == NODE_VARIABLE) {
        fprintf(file, "    ; ");
        node_print(file, node);
        fprintf(file, "\n");

        Local *local = node_find_local(currentBlock, node->string);
        if (inAssign) {
            if (arch == ARCH_ARM64) fprintf(file, "    sub x0, x8, %zu\n", local->offset);
            if (arch == ARCH_X86_64) fprintf(file, "    lea rax, [rbp - %zu]\n", local->offset);
        } else {
            if (arch == ARCH_ARM64) fprintf(file, "    ldr x0, [x8, -%zu]\n", local->offset);
            if (arch == ARCH_X86_64) fprintf(file, "    mov rax, [rbp - %zu]\n", local->offset);
        }
    }

    if (node->kind == NODE_FUNCCALL) {
        fprintf(file, "    ; ");
        node_print(file, node);
        fprintf(file, "\n");

        for (size_t i = 0; i < node->nodes->size; i++) {
            Node *statement = list_get(node->nodes, i);
            if (statement->kind != NODE_NULL) {
                node_asm(file, statement);
                fprintf(file, "\n");
                if (arch == ARCH_ARM64) fprintf(file, "    str x0, [sp, -16]!\n");
                if (arch == ARCH_X86_64) fprintf(file, "    push rax\n");
            }
        }

        if (arch == ARCH_ARM64) {
            for (int32_t i = (int32_t)node->nodes->size - 1; i >= 0; i--) {
                fprintf(file, "    ldr x%d, [sp], 16\n", i);
            }
            fprintf(file, "    str x30, [sp, -16]!\n");
            fprintf(file, "    bl _%s\n", node->string);
            fprintf(file, "    ldr x30, [sp], 16\n");
        }
        if (arch == ARCH_X86_64) {
            char *argregs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
            for (int32_t i = (int32_t)node->nodes->size - 1; i >= 0; i--) {
                fprintf(file, "    pop %s\n", argregs[i]);
            }
            fprintf(file, "    xor rax, rax\n");
            fprintf(file, "    call _%s\n", node->string);
        }
    }

    if (node->kind >= NODE_NEG && node->kind <= NODE_LOGIC_NOT) {
        fprintf(file, "    ; ");
        node_print(file, node);
        fprintf(file, "\n");

        if (node->kind == NODE_ADDR) {
            Local *local = node_find_local(currentBlock, node->unary->string);
            if (arch == ARCH_ARM64) fprintf(file, "    sub x0, x8, %zu\n", local->offset);
            if (arch == ARCH_X86_64) fprintf(file, "    lea rax, [rbp - %zu]\n", local->offset);
            return;
        }

        node_asm(file, node->unary);

        if (node->kind == NODE_NEG) {
            if (arch == ARCH_ARM64) fprintf(file, "    sub x0, xzr, x0\n");
            if (arch == ARCH_X86_64) fprintf(file, "    neg rax\n");
        }

        if (node->kind == NODE_DEREF) {
            if (!inAssign || node->unary->kind == NODE_VARIABLE) {
                if (arch == ARCH_ARM64) fprintf(file, "    ldr x0, [x0]\n");
                if (arch == ARCH_X86_64) fprintf(file, "    mov rax, [rax]\n");
            }
        }

        if (node->kind == NODE_LOGIC_NOT) {
            if (arch == ARCH_ARM64) {
                fprintf(file, "    cmp x0, 0\n");
                fprintf(file, "    cset x0, eq\n");
            }
            if (arch == ARCH_X86_64) {
                fprintf(file, "    cmp rax, 0\n");
                fprintf(file, "    sete al\n");
                fprintf(file, "    movzx rax, al\n");
            }
        }
    }

    if (node->kind >= NODE_ASSIGN && node->kind <= NODE_LOGIC_OR) {
        node_asm(file, node->rhs);

        if (!(node->kind == NODE_ASSIGN && node->rhs->kind == NODE_ASSIGN)) {
            if (arch == ARCH_ARM64) fprintf(file, "    str x0, [sp, -16]!\n");
            if (arch == ARCH_X86_64) fprintf(file, "    push rax\n");
        }

        if (node->kind == NODE_ASSIGN) inAssign = true;
        node_asm(file, node->lhs);
        if (node->kind == NODE_ASSIGN) inAssign = false;
        if (!(node->kind == NODE_ASSIGN && node->rhs->kind == NODE_ASSIGN)) {
            if (arch == ARCH_ARM64) fprintf(file, "    ldr x1, [sp], 16\n");
            if (arch == ARCH_X86_64) fprintf(file, "    pop rdx\n");
        }

        fprintf(file, "    ; ");
        node_print(file, node);
        fprintf(file, "\n");

        if (node->kind == NODE_ASSIGN) {
            if (arch == ARCH_ARM64) fprintf(file, "    str x1, [x0]\n");
            if (arch == ARCH_X86_64) fprintf(file, "    mov [rax], rdx\n");
        }
        if (node->kind == NODE_ADD) {
            if (arch == ARCH_ARM64) fprintf(file, "    add x0, x0, x1\n");
            if (arch == ARCH_X86_64) fprintf(file, "    add rax, rdx\n");
        }
        if (node->kind == NODE_SUB) {
            if (arch == ARCH_ARM64) fprintf(file, "    sub x0, x0, x1\n");
            if (arch == ARCH_X86_64) fprintf(file, "    sub rax, rdx\n");
        }
        if (node->kind == NODE_MUL) {
            if (arch == ARCH_ARM64) fprintf(file, "    mul x0, x0, x1\n");
            if (arch == ARCH_X86_64) fprintf(file, "    imul rdx\n");
        }
        if (node->kind == NODE_DIV) {
            if (arch == ARCH_ARM64) {
                fprintf(file, "    sdiv x0, x0, x1\n");
            }
            if (arch == ARCH_X86_64) {
                fprintf(file, "    mov rcx, rdx\n");
                fprintf(file, "    xor rdx, rdx\n");
                fprintf(file, "    idiv rcx\n");
            }
        }
        if (node->kind == NODE_MOD) {
            if (arch == ARCH_ARM64) {
                fprintf(file, "    udiv x2, x0, x1\n");
                fprintf(file, "    msub x0, x2, x1, x0\n");
            }
            if (arch == ARCH_X86_64) {
                fprintf(file, "    mov rcx, rdx\n");
                fprintf(file, "    xor rdx, rdx\n");
                fprintf(file, "    idiv rcx\n");
                fprintf(file, "    mov rax, rdx\n");
            }
        }

        if (node->kind >= NODE_EQ && node->kind <= NODE_GTEQ) {
            if (arch == ARCH_ARM64) fprintf(file, "    cmp x0, x1\n");
            if (arch == ARCH_X86_64) fprintf(file, "    cmp rax, rdx\n");

            if (node->kind == NODE_EQ) {
                if (arch == ARCH_ARM64) fprintf(file, "    cset x0, eq\n");
                if (arch == ARCH_X86_64) fprintf(file, "    sete al\n");
            }
            if (node->kind == NODE_NEQ) {
                if (arch == ARCH_ARM64) fprintf(file, "    cset x0, ne\n");
                if (arch == ARCH_X86_64) fprintf(file, "    setne al\n");
            }
            if (node->kind == NODE_LT) {
                if (arch == ARCH_ARM64) fprintf(file, "    cset x0, lt\n");
                if (arch == ARCH_X86_64) fprintf(file, "    setl al\n");
            }
            if (node->kind == NODE_LTEQ) {
                if (arch == ARCH_ARM64) fprintf(file, "    cset x0, le\n");
                if (arch == ARCH_X86_64) fprintf(file, "    setle al\n");
            }
            if (node->kind == NODE_GT) {
                if (arch == ARCH_ARM64) fprintf(file, "    cset x0, gt\n");
                if (arch == ARCH_X86_64) fprintf(file, "    setg al\n");
            }
            if (node->kind == NODE_GTEQ) {
                if (arch == ARCH_ARM64) fprintf(file, "    cset x0, ge\n");
                if (arch == ARCH_X86_64) fprintf(file, "    setge al\n");
            }

            if (arch == ARCH_X86_64) fprintf(file, "   movzx rax, al\n");
        }

        if (node->kind == NODE_LOGIC_AND) {
            if (arch == ARCH_ARM64) fprintf(file, "    and x0, x0, x1\n");
            if (arch == ARCH_X86_64) fprintf(file, "    and rax, rdx\n");
        }
        if (node->kind == NODE_LOGIC_OR) {
            if (arch == ARCH_ARM64) fprintf(file, "    or x0, x0, x1\n");
            if (arch == ARCH_X86_64) fprintf(file, "    or rax, rdx\n");
        }
    }
}

void codegen(FILE *file, Node *node, Arch _arch) {
    arch = _arch;
    node_asm(file, node);
}
