#include "node.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Node *node_new(NodeKind kind) {
    Node *node = malloc(sizeof(Node));
    node->kind = kind;
    return node;
}

Node *node_new_number(int64_t number) {
    Node *node = node_new(NODE_NUMBER);
    node->number = number;
    node->type = type_new(TYPE_NUMBER, 8, true);
    return node;
}

Node *node_new_unary(NodeKind kind, Node *unary) {
    Node *node = node_new(kind);
    node->unary = unary;
    node->type = unary->type;
    return node;
}

Node *node_new_operation(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = node_new(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    node->type = lhs->type;
    return node;
}

Node *node_new_multiple(NodeKind kind) {
    Node *node = node_new(kind);
    node->nodes = list_new(8);
    return node;
}

Local *node_find_local(Node *block, char *name) {
    if (block->parentBlock != NULL) {
        Local *local = node_find_local(block->parentBlock, name);
        if (local != NULL) {
            return local;
        }
    }

    for (size_t i = 0; i < block->locals->size; i++) {
        Local *local = list_get(block->locals, i);
        if (!strcmp(local->name, name)) {
            return local;
        }
    }
    return NULL;
}

void node_print(FILE *file, Node *node) {
    if (node->kind == NODE_MULTIPLE) {
        for (size_t i = 0; i < node->nodes->size; i++) {
            node_print(file, list_get(node->nodes, i));
            fprintf(file, "; ");
        }
        return;
    }
    if (node->kind == NODE_BLOCK) {
        fprintf(file, "{ ");
        for (size_t i = 0; i < node->locals->size; i++) {
            Local *local = list_get(node->locals, i);
            type_print(file, local->type);
            fprintf(file, " %s; ", local->name);
        }
        for (size_t i = 0; i < node->nodes->size; i++) {
            node_print(file, list_get(node->nodes, i));
            fprintf(file, "; ");
        }
        fprintf(file, " }");
        return;
    }

    if (node->kind == NODE_NUMBER) {
        fprintf(file, "%lld", node->number);
    }
    if (node->kind == NODE_VARIABLE) {
        fprintf(file, "%s", node->string);
    }
    if (node->kind == NODE_FNCALL) {
        fprintf(file, "%s(", node->string);
        for (size_t i = 0; i < node->nodes->size; i++) {
            node_print(file, list_get(node->nodes, i));
            if (i != node->nodes->size - 1) fprintf(file, ", ");
        }
        fprintf(file, ")");
    }

    if (node->kind == NODE_IF) {
        fprintf(file, "if (");
        type_print(file, node->condition->type);
        node_print(file, node->condition);
        fprintf(file, ") ");
        node_print(file, node->thenBlock);
        if (node->elseBlock != NULL) {
            fprintf(file, " else ");
            node_print(file, node->elseBlock);
        }
    }

    if (node->kind == NODE_WHILE) {
        fprintf(file, "while (");
        if (node->condition != NULL) {
            type_print(file, node->condition->type);
            node_print(file, node->condition);
        } else {
            fprintf(file, "1");
        }
        fprintf(file, ") ");
        node_print(file, node->thenBlock);
    }

    if (node->kind == NODE_RETURN) {
        fprintf(file, "return ");
        type_print(file, node->unary->type);
        node_print(file, node->unary);
    }

    if (node->kind >= NODE_NEG && node->kind <= NODE_LOGIC_NOT) {
        if (node->kind == NODE_NEG) fprintf(file, "(- ");
        if (node->kind == NODE_ADDR) fprintf(file, "(& ");
        if (node->kind == NODE_DEREF) fprintf(file, "(* ");
        if (node->kind == NODE_LOGIC_NOT) fprintf(file, "(! ");
        type_print(file, node->unary->type);
        node_print(file, node->unary);
        fprintf(file, ")");
    }

    if (node->kind >= NODE_ASSIGN && node->kind <= NODE_LOGIC_OR) {
        fprintf(file, "(");
        type_print(file, node->lhs->type);
        node_print(file, node->lhs);
        if (node->kind == NODE_ASSIGN) fprintf(file, " = ");
        if (node->kind == NODE_ADD) fprintf(file, " + ");
        if (node->kind == NODE_SUB) fprintf(file, " - ");
        if (node->kind == NODE_MUL) fprintf(file, " * ");
        if (node->kind == NODE_DIV) fprintf(file, " / ");
        if (node->kind == NODE_MOD) fprintf(file, " %% ");
        if (node->kind == NODE_EQ) fprintf(file, " == ");
        if (node->kind == NODE_NEQ) fprintf(file, " != ");
        if (node->kind == NODE_LT) fprintf(file, " < ");
        if (node->kind == NODE_LTEQ) fprintf(file, " <= ");
        if (node->kind == NODE_GT) fprintf(file, " > ");
        if (node->kind == NODE_GTEQ) fprintf(file, " >= ");
        if (node->kind == NODE_LOGIC_AND) fprintf(file, " && ");
        if (node->kind == NODE_LOGIC_OR) fprintf(file, " || ");
        type_print(file, node->rhs->type);
        node_print(file, node->rhs);
        fprintf(file, ")");
    }
}
