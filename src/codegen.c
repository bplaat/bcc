#include "codegen.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "type.h"

void codegen(FILE *file, Arch *arch, Node *node) {
    Codegen codegen;
    codegen.file = file;
    codegen.arch = arch;
    codegen.regsUsed = malloc(sizeof(bool) * arch->regsSize);
    codegen.nestedAssign = false;
    codegen_node(&codegen, node, -1);
}

int32_t codegen_alloc(Codegen *codegen, int32_t requestReg) {
    if (requestReg != -1 && !codegen->regsUsed[requestReg]) {
        codegen->regsUsed[requestReg] = true;
        return requestReg;
    }
    for (int32_t i = 0; i < codegen->arch->regsSize; i++) {
        if (!codegen->regsUsed[i]) {
            codegen->regsUsed[i] = true;
            return i;
        }
    }
    fprintf(stderr, "All regs are used up\n");
    exit(1);
}

void codegen_free(Codegen *codegen, int32_t reg) { codegen->regsUsed[reg] = false; }

int32_t codegen_node(Codegen *codegen, Node *node, int32_t requestReg) {
    FILE *f = codegen->file;
    Arch *arch = codegen->arch;

    if (node->kind == NODE_PROGRAM) {
        for (size_t i = 0; i < node->nodes->size; i++) {
            codegen_node(codegen, list_get(node->nodes, i), -1);
        }
    }

    if (node->kind == NODE_FUNCDEF) {
        fprintf(f, "\n.global _%s\n", node->funcname);
        fprintf(f, "_%s:\n", node->funcname);
        if (!node->isLeaf && arch->kind == ARCH_ARM64) fprintf(f, "    push lr\n");

        if (node->locals->size > 0) {
            size_t stackSize = align(((Local *)list_get(node->locals, 0))->offset, arch->stackAlign);
            if (arch->kind == ARCH_ARM64) {
                fprintf(f, "    push fp\n");
                fprintf(f, "    mov fp, sp\n");
                fprintf(f, "    sub sp, sp, %zu\n", stackSize);
            }
            if (arch->kind == ARCH_X86_64) {
                fprintf(f, "    push rbp\n");
                fprintf(f, "    mov rbp, rsp\n");
                fprintf(f, "    sub rsp, %zu\n", stackSize);
            }
        }

        for (int32_t i = (int32_t)node->argsSize - 1; i >= 0; i--) {
            Local *local = list_get(node->locals, i);
            if (type_is_8(local->type)) {
                if (arch->kind == ARCH_ARM64) fprintf(f, "    strb %s, [fp, -%zu]\n", arch->regs32[arch->argRegs[i]], local->offset);
                if (arch->kind == ARCH_X86_64) fprintf(f, "    mov byte ptr [rbp - %zu], %s\n", local->offset, arch->regs8[arch->argRegs[i]]);
            }
            if (type_is_16(local->type)) {
                if (arch->kind == ARCH_ARM64) fprintf(f, "    strh %s, [fp, -%zu]\n", arch->regs32[arch->argRegs[i]], local->offset);
                if (arch->kind == ARCH_X86_64) fprintf(f, "    mov word ptr [rbp - %zu], %s\n", local->offset, arch->regs16[arch->argRegs[i]]);
            }
            if (type_is_32(local->type)) {
                if (arch->kind == ARCH_ARM64) fprintf(f, "    str %s, [fp, -%zu]\n", arch->regs32[arch->argRegs[i]], local->offset);
                if (arch->kind == ARCH_X86_64) fprintf(f, "    mov dword ptr [rbp - %zu], %s\n", local->offset, arch->regs32[arch->argRegs[i]]);
            }
            if (type_is_64(local->type)) {
                if (arch->kind == ARCH_ARM64) fprintf(f, "    str %s, [fp, -%zu]\n", arch->regs64[arch->argRegs[i]], local->offset);
                if (arch->kind == ARCH_X86_64) fprintf(f, "    mov qword ptr [rbp - %zu], %s\n", local->offset, arch->regs64[arch->argRegs[i]]);
            }
        }

        codegen->currentFuncdef = node;
        codegen->uniqueLabel = 1;
        for (size_t i = 0; i < node->nodes->size; i++) {
            Node *child = list_get(node->nodes, i);
            fprintf(f, "    // %s\n", node_to_string(child));
            codegen_node(codegen, child, -1);
            fprintf(f, "\n");
        }

        if (node->nodes->size == 0 || ((Node *)list_get(node->nodes, node->nodes->size - 1))->kind != NODE_RETURN) {
            if (node->locals->size > 0) {
                if (arch->kind == ARCH_ARM64) {
                    fprintf(f, "    mov sp, fp\n");
                    fprintf(f, "    pop fp\n");
                }
                if (arch->kind == ARCH_X86_64) {
                    fprintf(f, "    leave\n");
                }
            }
            if (arch->kind == ARCH_ARM64 && !node->isLeaf) {
                fprintf(f, "    pop lr\n");
            }
            fprintf(f, "    ret\n");
        }
    }

    if (node->kind == NODE_BLOCK) {
        for (size_t i = 0; i < node->nodes->size; i++) {
            Node *child = list_get(node->nodes, i);
            fprintf(f, "    // %s\n", node_to_string(child));
            codegen_node(codegen, child, -1);
            fprintf(f, "\n");
        }
    }

    if (node->kind == NODE_INTEGER) {
        int32_t reg = codegen_alloc(codegen, requestReg);
        if (arch->kind == ARCH_ARM64) {
            if (node->integer < 0) {
                fprintf(f, "    mov %s, %lld\n", arch->regs64[reg], -node->integer & 0xffff);
                if (node->integer >= 0xffff) {
                    fprintf(f, "    movk %s, %lld, lsl 16\n", arch->regs64[reg], (-node->integer >> 16) & 0xffff);
                }
                fprintf(f, "    sub %s, xzr, %s\n", arch->regs64[reg], arch->regs64[reg]);
            } else {
                fprintf(f, "    mov %s, %lld\n", arch->regs64[reg], node->integer & 0xffff);
                if (node->integer >= 0xffff) {
                    fprintf(f, "    movk %s, %lld, lsl 16\n", arch->regs64[reg], (node->integer >> 16) & 0xffff);
                }
            }
        }
        if (arch->kind == ARCH_X86_64) {
            if (node->integer == 0) {
                fprintf(f, "    xor %s, %s\n", arch->regs32[reg], arch->regs32[reg]);
            } else {
                fprintf(f, "    mov %s, %lld\n", arch->regs64[reg], node->integer);
            }
        }
        return reg;
    }
    if (node->kind == NODE_LOCAL) {
        int32_t reg = codegen_alloc(codegen, requestReg);
        if (node->type->kind == TYPE_ARRAY) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    sub %s, fp, %zu\n", arch->regs64[reg], node->local->offset);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    lea %s, [rbp - %zu]\n", arch->regs64[reg], node->local->offset);
        } else if (type_is_8(node->type)) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    ldrb %s, [fp, -%zu]\n", arch->regs32[reg], node->local->offset);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    movzx %s, byte ptr [rbp - %zu]\n", arch->regs8[reg], node->local->offset);
        } else if (type_is_16(node->type)) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    ldrh %s, [fp, -%zu]\n", arch->regs32[reg], node->local->offset);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    movzx %s, word ptr [rbp - %zu]\n", arch->regs16[reg], node->local->offset);
        } else if (type_is_32(node->type)) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    ldr %s, [fp, -%zu]\n", arch->regs32[reg], node->local->offset);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    mov %s, dword ptr [rbp - %zu]\n", arch->regs32[reg], node->local->offset);
        } else if (type_is_64(node->type)) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    ldr %s, [fp, -%zu]\n", arch->regs64[reg], node->local->offset);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    mov %s, qword ptr [rbp - %zu]\n", arch->regs64[reg], node->local->offset);
        }
        return reg;
    }
    if (node->kind == NODE_FUNCCALL) {
        bool regsPushed[16] = {0};
        for (int32_t i = arch->regsSize - 1; i >= 0; i--) {
            if (codegen->regsUsed[i]) {
                regsPushed[i] = true;
                fprintf(f, "    push %s\n", arch->regs64[i]);
                codegen_free(codegen, i);
            }
        }
        for (int32_t i = (int32_t)node->nodes->size - 1; i >= 0; i--) {
            codegen_node(codegen, list_get(node->nodes, i), arch->argRegs[i]);
        }

        if (arch->kind == ARCH_ARM64) fprintf(f, "    bl _%s\n", node->funcname);
        if (arch->kind == ARCH_X86_64) fprintf(f, "    call _%s\n", node->funcname);

        for (int32_t i = 0; i < arch->regsSize; i++) {
            codegen->regsUsed[i] = regsPushed[i];
        }

        int32_t returnReg = codegen_alloc(codegen, requestReg);
        if (returnReg != arch->returnReg) {
            fprintf(f, "    mov %s, %s\n", arch->regs64[returnReg], arch->regs64[arch->returnReg]);
        }
        for (int32_t i = 0; i < arch->regsSize; i++) {
            if (regsPushed[i]) {
                fprintf(f, "    pop %s\n", arch->regs64[i]);
            }
        }
        return returnReg;
    }

    if (node->kind == NODE_IF) {
        int32_t skipLabel = codegen->uniqueLabel++;
        int32_t elseLabel;
        if (node->elseBlock != NULL) elseLabel = codegen->uniqueLabel++;

        int32_t reg = codegen_node(codegen, node->condition, -1);
        if (arch->kind == ARCH_ARM64) fprintf(f, "    cbz %s, .L%d\n", arch->regs64[reg], node->elseBlock != NULL ? elseLabel : skipLabel);
        if (arch->kind == ARCH_X86_64) {
            fprintf(f, "    cmp %s, 0\n", arch->regs64[reg]);
            fprintf(f, "    je .L%d\n", node->elseBlock != NULL ? elseLabel : skipLabel);
        }
        codegen_free(codegen, reg);

        codegen_node(codegen, node->thenBlock, -1);
        if (node->elseBlock != NULL) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    b .L%d\n", skipLabel);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    jmp .L%d\n", skipLabel);
            fprintf(f, ".L%d:\n", elseLabel);
            codegen_node(codegen, node->elseBlock, -1);
        }
        fprintf(f, ".L%d:\n", skipLabel);
        return -1;
    }

    if (node->kind == NODE_WHILE) {
        int32_t repeatLabel = codegen->uniqueLabel++;
        int32_t breakLabel;
        if (node->condition != NULL) breakLabel = codegen->uniqueLabel++;

        fprintf(f, ".L%d:\n", repeatLabel);
        if (node->condition != NULL) {
            int32_t reg = codegen_node(codegen, node->condition, -1);
            if (arch->kind == ARCH_ARM64) fprintf(f, "    cbz %s, .L%d\n", arch->regs64[reg], breakLabel);
            if (arch->kind == ARCH_X86_64) {
                fprintf(f, "    cmp %s, 0\n", arch->regs64[reg]);
                fprintf(f, "    je .L%d\n", breakLabel);
            }
            codegen_free(codegen, reg);
        }

        codegen_node(codegen, node->thenBlock, -1);
        if (arch->kind == ARCH_ARM64) fprintf(f, "    b .L%d\n", repeatLabel);
        if (arch->kind == ARCH_X86_64) fprintf(f, "    jmp .L%d\n", repeatLabel);
        if (node->condition != NULL) fprintf(f, ".L%d:\n", breakLabel);
        return -1;
    }

    if (node->kind == NODE_RETURN) {
        int32_t reg = codegen_node(codegen, node->unary, arch->returnReg);
        if (arch->kind == ARCH_ARM64 && reg != 0) fprintf(f, "    mov x0, %s\n", arch->regs64[reg]);
        if (arch->kind == ARCH_X86_64 && reg != 0) fprintf(f, "    mov rax, %s\n", arch->regs64[reg]);

        if (codegen->currentFuncdef->locals->size > 0) {
            if (arch->kind == ARCH_ARM64) {
                fprintf(f, "    mov sp, fp\n");
                fprintf(f, "    pop fp\n");
            }
            if (arch->kind == ARCH_X86_64) {
                fprintf(f, "    leave\n");
            }
        }
        if (arch->kind == ARCH_ARM64 && !codegen->currentFuncdef->isLeaf) {
            fprintf(f, "    pop lr\n");
        }
        fprintf(f, "    ret\n");
        codegen_free(codegen, reg);
    }

    if ((node->kind >= NODE_NEG && node->kind <= NODE_LOGIC_NOT) || node->kind == NODE_DEREF) {
        int32_t reg = codegen_node(codegen, node->unary, requestReg);
        if (node->kind == NODE_NEG) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    sub %s, xzr, %s\n", arch->regs64[reg], arch->regs64[reg]);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    neg %s\n", arch->regs64[reg]);
        }
        if (node->kind == NODE_LOGIC_NOT) {
            if (arch->kind == ARCH_ARM64) {
                fprintf(f, "    cmp %s, 0\n", arch->regs64[reg]);
                fprintf(f, "    cset %s, eq\n", arch->regs64[reg]);
            }
            if (arch->kind == ARCH_X86_64) {
                fprintf(f, "    cmp %s, 0\n", arch->regs64[reg]);
                fprintf(f, "    sete %s\n", arch->regs8[reg]);
                fprintf(f, "    movzx %s, %s\n", arch->regs64[reg], arch->regs8[reg]);
            }
        }
        if (node->kind == NODE_DEREF && node->type->kind != TYPE_ARRAY) {
            if (type_is_8(node->type)) {
                if (arch->kind == ARCH_ARM64) fprintf(f, "    ldrb %s, [%s]\n", arch->regs32[reg], arch->regs64[reg]);
                if (arch->kind == ARCH_X86_64) fprintf(f, "    movzx %s, byte ptr [%s]\n", arch->regs8[reg], arch->regs64[reg]);
            }
            if (type_is_16(node->type)) {
                if (arch->kind == ARCH_ARM64) fprintf(f, "    ldrh %s, [%s]\n", arch->regs32[reg], arch->regs64[reg]);
                if (arch->kind == ARCH_X86_64) fprintf(f, "    movzx %s, word ptr [%s]\n", arch->regs16[reg], arch->regs64[reg]);
            }
            if (type_is_32(node->type)) {
                if (arch->kind == ARCH_ARM64) fprintf(f, "    ldr %s, [%s]\n", arch->regs32[reg], arch->regs64[reg]);
                if (arch->kind == ARCH_X86_64) fprintf(f, "    mov %s, dword ptr [%s]\n", arch->regs32[reg], arch->regs64[reg]);
            }
            if (type_is_64(node->type)) {
                if (arch->kind == ARCH_ARM64) fprintf(f, "    ldr %s, [%s]\n", arch->regs64[reg], arch->regs64[reg]);
                if (arch->kind == ARCH_X86_64) fprintf(f, "    mov %s, qword ptr [%s]\n", arch->regs64[reg], arch->regs64[reg]);
            }
        }
        return reg;
    }
    if (node->kind == NODE_REF) {
        int32_t reg = codegen_alloc(codegen, requestReg);
        if (arch->kind == ARCH_ARM64) fprintf(f, "    sub %s, fp, %zu\n", arch->regs64[reg], node->unary->local->offset);
        if (arch->kind == ARCH_X86_64) fprintf(f, "    lea %s, [rbp - %zu]\n", arch->regs64[reg], node->unary->local->offset);
        return reg;
    }

    if (node->kind == NODE_ASSIGN) {
        if (!codegen->nestedAssign && arch->kind == ARCH_X86_64 && node->rhs->kind == NODE_INTEGER) {
            if (type_is_8(node->type)) fprintf(f, "    mov byte ptr [rbp - %zu], %lld\n", node->lhs->local->offset, node->rhs->integer);
            if (type_is_16(node->type)) fprintf(f, "    mov word ptr [rbp - %zu], %lld\n", node->lhs->local->offset, node->rhs->integer);
            if (type_is_32(node->type)) fprintf(f, "    mov dword ptr [rbp - %zu], %lld\n", node->lhs->local->offset, node->rhs->integer);
            if (type_is_64(node->type)) fprintf(f, "    mov qword ptr [rbp - %zu], %lld\n", node->lhs->local->offset, node->rhs->integer);
            return -1;
        }

        if (node->rhs->kind == NODE_ASSIGN) codegen->nestedAssign = true;
        int32_t rhsReg = codegen_node(codegen, node->rhs, requestReg);
        if (type_is_8(node->type)) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    strb %s, [fp, -%zu]\n", arch->regs32[rhsReg], node->lhs->local->offset);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    mov byte ptr [rbp - %zu], %s\n", node->lhs->local->offset, arch->regs8[rhsReg]);
        }
        if (type_is_16(node->type)) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    strh %s, [fp, -%zu]\n", arch->regs32[rhsReg], node->lhs->local->offset);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    mov word ptr [rbp - %zu], %s\n", node->lhs->local->offset, arch->regs16[rhsReg]);
        }
        if (type_is_32(node->type)) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    str %s, [fp, -%zu]\n", arch->regs32[rhsReg], node->lhs->local->offset);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    mov dword ptr [rbp - %zu], %s\n", node->lhs->local->offset, arch->regs32[rhsReg]);
        }
        if (type_is_64(node->type)) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    str %s, [fp, -%zu]\n", arch->regs64[rhsReg], node->lhs->local->offset);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    mov qword ptr [rbp - %zu], %s\n", node->lhs->local->offset, arch->regs64[rhsReg]);
        }
        if (codegen->nestedAssign) {
            codegen->nestedAssign = false;
            return rhsReg;
        } else {
            codegen_free(codegen, rhsReg);
            return -1;
        }
    }
    if (node->kind == NODE_ASSIGN_PTR) {
        if (!codegen->nestedAssign && arch->kind == ARCH_X86_64 && node->rhs->kind == NODE_INTEGER) {
            int32_t lhsReg = codegen_node(codegen, node->lhs, requestReg);
            if (type_is_8(node->type)) fprintf(f, "    mov byte ptr [%s], %lld\n", arch->regs64[lhsReg], node->rhs->integer);
            if (type_is_16(node->type)) fprintf(f, "    mov word ptr [%s], %lld\n", arch->regs64[lhsReg], node->rhs->integer);
            if (type_is_32(node->type)) fprintf(f, "    mov dword ptr [%s], %lld\n", arch->regs64[lhsReg], node->rhs->integer);
            if (type_is_64(node->type)) fprintf(f, "    mov qword ptr [%s], %lld\n", arch->regs64[lhsReg], node->rhs->integer);
            codegen_free(codegen, lhsReg);
            return -1;
        }

        if (node->rhs->kind == NODE_ASSIGN) codegen->nestedAssign = true;
        int32_t lhsReg = codegen_node(codegen, node->lhs, requestReg);
        int32_t rhsReg = codegen_node(codegen, node->rhs, -1);
        if (type_is_8(node->type)) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    strb %s, [%s]\n", arch->regs32[rhsReg], arch->regs64[lhsReg]);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    mov byte ptr [%s], %s\n", arch->regs64[lhsReg], arch->regs8[rhsReg]);
        }
        if (type_is_16(node->type)) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    strh %s, [%s]\n", arch->regs32[rhsReg], arch->regs64[lhsReg]);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    mov word ptr [%s], %s\n", arch->regs64[lhsReg], arch->regs16[rhsReg]);
        }
        if (type_is_32(node->type)) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    str %s, [%s]\n", arch->regs32[rhsReg], arch->regs64[lhsReg]);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    mov dword ptr [%s], %s\n", arch->regs64[lhsReg], arch->regs32[rhsReg]);
        }
        if (type_is_64(node->type)) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    str %s, [%s]\n", arch->regs64[rhsReg], arch->regs64[lhsReg]);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    mov qword ptr [%s], %s\n", arch->regs64[lhsReg], arch->regs64[rhsReg]);
        }
        if (codegen->nestedAssign) {
            codegen->nestedAssign = false;
            return rhsReg;
        } else {
            codegen_free(codegen, rhsReg);
            return -1;
        }
    }

    if (node->kind >= NODE_ADD && node->kind <= NODE_LOGIC_AND) {
        int32_t lhsReg = codegen_node(codegen, node->lhs, requestReg);
        if (node->kind == NODE_ADD && node->rhs->kind == NODE_INTEGER) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    add %s, %s, %lld\n", arch->regs64[lhsReg], arch->regs64[lhsReg], node->rhs->integer);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    add %s, %lld\n", arch->regs64[lhsReg], node->rhs->integer);
            return lhsReg;
        }
        if (node->kind == NODE_SUB && node->rhs->kind == NODE_INTEGER) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    sub %s, %s, %lld\n", arch->regs64[lhsReg], arch->regs64[lhsReg], node->rhs->integer);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    sub %s, %lld\n", arch->regs64[lhsReg], node->rhs->integer);
            return lhsReg;
        }
        if (arch->kind == ARCH_X86_64 && node->kind == NODE_MUL && node->rhs->kind == NODE_INTEGER) {
            fprintf(f, "    imul %s, %lld\n", arch->regs64[lhsReg], node->rhs->integer);
            return lhsReg;
        }

        int32_t rhsReg = codegen_node(codegen, node->rhs, -1);
        if (node->kind == NODE_ADD) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    add %s, %s, %s\n", arch->regs64[lhsReg], arch->regs64[lhsReg], arch->regs64[rhsReg]);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    add %s, %s\n", arch->regs64[lhsReg], arch->regs64[rhsReg]);
        }
        if (node->kind == NODE_SUB) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    sub %s, %s, %s\n", arch->regs64[lhsReg], arch->regs64[lhsReg], arch->regs64[rhsReg]);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    sub %s, %s\n", arch->regs64[lhsReg], arch->regs64[rhsReg]);
        }
        if (node->kind == NODE_MUL) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    mul %s, %s, %s\n", arch->regs64[lhsReg], arch->regs64[lhsReg], arch->regs64[rhsReg]);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    imul %s, %s\n", arch->regs64[lhsReg], arch->regs64[rhsReg]);
        }
        if (node->kind == NODE_DIV) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    sdiv %s, %s, %s\n", arch->regs64[lhsReg], arch->regs64[lhsReg], arch->regs64[rhsReg]);
            if (arch->kind == ARCH_X86_64) {
                if (lhsReg != 0) fprintf(f, "    mov r12, rax\n");
                if (lhsReg != 3 && codegen->regsUsed[3]) fprintf(f, "    mov r13, rdx\n");
                if (lhsReg != 0) fprintf(f, "    mov rax, %s\n", arch->regs64[lhsReg]);
                fprintf(f, "    cqo\n");
                fprintf(f, "    idiv %s\n", arch->regs64[rhsReg]);
                if (lhsReg != 0) fprintf(f, "    mov %s, rax\n", arch->regs64[lhsReg]);
                if (lhsReg != 0) fprintf(f, "    mov rax, r12\n");
                if (lhsReg != 3 && codegen->regsUsed[3]) fprintf(f, "    mov rdx, r13\n");
                ;
            }
        }
        if (node->kind == NODE_MOD) {
            if (arch->kind == ARCH_ARM64) {
                fprintf(f, "    udiv x8, %s, %s\n", arch->regs64[lhsReg], arch->regs64[rhsReg]);
                fprintf(f, "    msub %s, x8, %s, %s\n", arch->regs64[lhsReg], arch->regs64[rhsReg], arch->regs64[lhsReg]);
            }
            if (arch->kind == ARCH_X86_64) {
                if (lhsReg != 0) fprintf(f, "    mov r12, rax\n");
                if (lhsReg != 3 && codegen->regsUsed[3]) fprintf(f, "    mov r13, rdx\n");
                if (lhsReg != 0) fprintf(f, "    mov rax, %s\n", arch->regs64[lhsReg]);
                fprintf(f, "    cqo\n");
                fprintf(f, "    idiv %s\n", arch->regs64[rhsReg]);
                if (lhsReg != 3) fprintf(f, "    mov %s, rdx\n", arch->regs64[lhsReg]);
                if (lhsReg != 0) fprintf(f, "    mov rax, r12\n");
                if (lhsReg != 3 && codegen->regsUsed[3]) fprintf(f, "    mov rdx, r13\n");
            }
        }

        if (node->kind >= NODE_EQ && node->kind <= NODE_GTEQ) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    cmp %s, %s\n", arch->regs64[lhsReg], arch->regs64[rhsReg]);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    cmp %s, %s\n", arch->regs64[lhsReg], arch->regs64[rhsReg]);
            if (node->kind == NODE_EQ) {
                if (arch->kind == ARCH_ARM64) fprintf(f, "    cset %s, eq\n", arch->regs64[lhsReg]);
                if (arch->kind == ARCH_X86_64) fprintf(f, "    sete %s\n", arch->regs8[lhsReg]);
            }
            if (node->kind == NODE_NEQ) {
                if (arch->kind == ARCH_ARM64) fprintf(f, "    cset %s, ne\n", arch->regs64[lhsReg]);
                if (arch->kind == ARCH_X86_64) fprintf(f, "    setne %s\n", arch->regs8[lhsReg]);
            }
            if (node->kind == NODE_LT) {
                if (arch->kind == ARCH_ARM64) fprintf(f, "    cset %s, lt\n", arch->regs64[lhsReg]);
                if (arch->kind == ARCH_X86_64) fprintf(f, "    setl %s\n", arch->regs8[lhsReg]);
            }
            if (node->kind == NODE_LTEQ) {
                if (arch->kind == ARCH_ARM64) fprintf(f, "    cset %s, le\n", arch->regs64[lhsReg]);
                if (arch->kind == ARCH_X86_64) fprintf(f, "    setle %s\n", arch->regs8[lhsReg]);
            }
            if (node->kind == NODE_GT) {
                if (arch->kind == ARCH_ARM64) fprintf(f, "    cset %s, gt\n", arch->regs64[lhsReg]);
                if (arch->kind == ARCH_X86_64) fprintf(f, "    setg %s\n", arch->regs8[lhsReg]);
            }
            if (node->kind == NODE_GTEQ) {
                if (arch->kind == ARCH_ARM64) fprintf(f, "    cset %s, ge\n", arch->regs64[lhsReg]);
                if (arch->kind == ARCH_X86_64) fprintf(f, "    setge %s\n", arch->regs8[lhsReg]);
            }
            if (arch->kind == ARCH_X86_64) fprintf(f, "    movzx %s, %s\n", arch->regs64[lhsReg], arch->regs8[lhsReg]);
        }
        if (node->kind == NODE_LOGIC_AND) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    and %s, %s, %s\n", arch->regs64[lhsReg], arch->regs64[lhsReg], arch->regs64[rhsReg]);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    and %s, %s\n", arch->regs64[lhsReg], arch->regs64[rhsReg]);
        }
        if (node->kind == NODE_LOGIC_OR) {
            if (arch->kind == ARCH_ARM64) fprintf(f, "    orr %s, %s, %s\n", arch->regs64[lhsReg], arch->regs64[lhsReg], arch->regs64[rhsReg]);
            if (arch->kind == ARCH_X86_64) fprintf(f, "    or %s, %s\n", arch->regs64[lhsReg], arch->regs64[rhsReg]);
        }
        codegen_free(codegen, rhsReg);
        return lhsReg;
    }
    return -1;
}
