#include "node.h"
#include <stdlib.h>
#include "utils.h"

Node *node_new(NodeKind kind) {
    Node *node = malloc(sizeof(Node));
    node->kind = kind;
    return node;
}

Node *node_new_integer(int64_t integer) {
    Node *node = node_new(NODE_INTEGER);
    node->integer = integer;
    return node;
}

Node *node_new_string(NodeKind kind, char *string) {
    Node *node = node_new(kind);
    node->string = string;
    return node;
}

Node *node_new_unary(NodeKind kind, Node *unary) {
    Node *node = node_new(kind);
    node->unary = unary;
    return node;
}

Node *node_new_operation(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = node_new(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *node_new_multiple(NodeKind kind) {
    Node *node = node_new(kind);
    node->nodes = list_new(8);
    return node;
}

Node *node_new_register(int32_t reg, int32_t size) {
    Node *node = node_new(NODE_REGISTER);
    node->reg = reg;
    node->size = size;
    return node;
}

bool node_is_calcable(Node *node) {
    return node->kind == NODE_INTEGER || node->kind == NODE_ADD || node->kind == NODE_SUB ||
        node->kind == NODE_MUL || node->kind == NODE_DIV || node->kind == NODE_MOD;
}

char *node_to_string(Node *node) {
    if (node->kind == NODE_PROGRAM) {
        List *sb = list_new(8);
        for (size_t i = 0; i < node->nodes->size; i++) {
            list_add(sb, node_to_string(list_get(node->nodes, i)));
            list_add(sb, "\n");
        }
        return list_to_string(sb);
    }
    if (node->kind == NODE_SECTION) {
        return format("section %s", node->string);
    }
    if (node->kind == NODE_LABEL) {
        return format("%s:", node->string);
    }
    if (node->kind == NODE_INSTRUCTION) {
        List *sb = list_new(8);
        list_add(sb, token_kind_to_string(node->opcode));
        list_add(sb, " ");
        for (size_t i = 0; i < node->nodes->size; i++) {
            list_add(sb, node_to_string(list_get(node->nodes, i)));
            if (i != node->nodes->size - 1) list_add(sb, ", ");
        }
        return list_to_string(sb);
    }
    if (node->kind == NODE_TIMES) {
        List *sb = list_new(8);
        list_add(sb, "times ");
        list_add(sb, node_to_string(node->lhs));
        list_add(sb, " ");
        list_add(sb, node_to_string(node->rhs));
        return list_to_string(sb);
    }

    if (node->kind == NODE_INTEGER) {
        return format("%lld", node->integer);
    }
    if (node->kind == NODE_STRING) {
        List *sb = list_new(8);
        list_add(sb, "\"");
        list_add(sb, node->string);
        list_add(sb, "\"");
        return list_to_string(sb);
    }
    if (node->kind == NODE_KEYWORD) {
        return strdup(node->string);
    }
    if (node->kind == NODE_ADDR) {
        List *sb = list_new(8);
        if (node->size == 8) list_add(sb, "byte");
        if (node->size == 16) list_add(sb, "word");
        if (node->size == 32) list_add(sb, "dword");
        if (node->size == 64) list_add(sb, "qword");
        list_add(sb, " ptr [");
        list_add(sb, node_to_string(node->unary));
        list_add(sb, "]");
        return list_to_string(sb);
    }

    if (node->kind == NODE_NEG) {
        List *sb = list_new(8);
        if (node->kind == NODE_NEG) list_add(sb, "(- ");
        list_add(sb, node_to_string(node->unary));
        list_add(sb, ")");
        return list_to_string(sb);
    }

    if (node->kind >= NODE_ADD && node->kind <= NODE_MOD) {
        List *sb = list_new(8);
        list_add(sb, "(");
        list_add(sb, node_to_string(node->lhs));
        if (node->kind == NODE_ADD) list_add(sb, " + ");
        if (node->kind == NODE_SUB) list_add(sb, " - ");
        if (node->kind == NODE_MUL) list_add(sb, " * ");
        if (node->kind == NODE_DIV) list_add(sb, " / ");
        if (node->kind == NODE_MOD) list_add(sb, " %% ");
        list_add(sb, node_to_string(node->rhs));
        list_add(sb, ")");
        return list_to_string(sb);
    }

    if (node->kind == NODE_REGISTER) {
        if (node->size == 32) {
            if (node->reg == 0) return strdup("eax");
            if (node->reg == 1) return strdup("ecx");
            if (node->reg == 2) return strdup("edx");
            if (node->reg == 3) return strdup("ebx");
            if (node->reg == 4) return strdup("esp");
            if (node->reg == 5) return strdup("ebp");
            if (node->reg == 6) return strdup("esi");
            if (node->reg == 7) return strdup("edi");
        }
        if (node->size == 64) {
            if (node->reg == 0) return strdup("rax");
            if (node->reg == 1) return strdup("rcx");
            if (node->reg == 2) return strdup("rdx");
            if (node->reg == 3) return strdup("rbx");
            if (node->reg == 4) return strdup("rsp");
            if (node->reg == 5) return strdup("rbp");
            if (node->reg == 6) return strdup("rsi");
            if (node->reg == 7) return strdup("rdi");
        }
    }
    return NULL;
}
