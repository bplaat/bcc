#include "node.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Local *local_new(char *string, Type *type) {
    Local *local = malloc(sizeof(Local));
    local->name = string;
    local->type = type;
    return local;
}

Node *node_new(NodeKind kind) {
    Node *node = malloc(sizeof(Node));
    node->kind = kind;
    return node;
}

Node *node_new_integer(int64_t integer) {
    Node *node = node_new(NODE_INTEGER);
    node->integer = integer;
    node->type = type_new(TYPE_INTEGER, 4, true);
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
    if (block->kind == NODE_BLOCK && block->parentBlock != NULL) {
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
    static int indentDepth = 0;

    if (node->kind == NODE_PROGRAM) {
        for (size_t i = 0; i < node->funcs->size; i++) {
            Node *functionNode = list_get(node->funcs, i);
            node_print(file, functionNode);
            fprintf(file, "\n");
        }
    }

    if (node->kind == NODE_MULTIPLE) {
        for (size_t i = 0; i < node->nodes->size; i++) {
            Node *childNode = list_get(node->nodes, i);
            if (childNode->kind == NODE_NULL) continue;
            if (childNode->kind != NODE_MULTIPLE && childNode->kind != NODE_BLOCK) {
                for (int32_t i = 0; i < indentDepth * 4; i++) fprintf(file, " ");
            }
            node_print(file, childNode);
            if (childNode->kind != NODE_MULTIPLE && childNode->kind != NODE_BLOCK && childNode->kind != NODE_IF &&
                childNode->kind != NODE_WHILE) {
                fprintf(file, ";\n");
            }
        }
    }

    if (node->kind == NODE_FUNCDEF) {
        type_print(file, node->type);
        fprintf(file, " %s(", node->functionName);
        for (size_t i = 0; i < node->argsSize; i++) {
            Local *local = list_get(node->locals, i);
            type_print(file, local->type);
            fprintf(file, " %s", local->name);
            if (i != node->argsSize - 1) fprintf(file, ", ");
        }
        fprintf(file, ") {\n");
        indentDepth++;
        for (size_t i = node->argsSize; i < node->locals->size; i++) {
            Local *local = list_get(node->locals, i);
            for (int32_t i = 0; i < indentDepth * 4; i++) fprintf(file, " ");
            type_print(file, local->type);
            fprintf(file, " %s;\n", local->name);
        }

        for (size_t i = 0; i < node->nodes->size; i++) {
            Node *childNode = list_get(node->nodes, i);
            if (childNode->kind == NODE_NULL) continue;
            if (childNode->kind != NODE_MULTIPLE && childNode->kind != NODE_BLOCK) {
                for (int32_t i = 0; i < indentDepth * 4; i++) fprintf(file, " ");
            }
            node_print(file, childNode);
            if (childNode->kind != NODE_MULTIPLE && childNode->kind != NODE_BLOCK && childNode->kind != NODE_IF &&
                childNode->kind != NODE_WHILE) {
                fprintf(file, ";\n");
            }
        }
        indentDepth--;
        fprintf(file, "}\n");
    }

    if (node->kind == NODE_BLOCK) {
        for (int32_t i = 0; i < indentDepth * 4; i++) fprintf(file, " ");
        fprintf(file, "{\n");
        indentDepth++;
        for (size_t i = 0; i < node->locals->size; i++) {
            Local *local = list_get(node->locals, i);
            for (int32_t i = 0; i < indentDepth * 4; i++) fprintf(file, " ");
            type_print(file, local->type);
            fprintf(file, " %s;\n", local->name);
        }

        for (size_t i = 0; i < node->nodes->size; i++) {
            Node *childNode = list_get(node->nodes, i);
            if (childNode->kind == NODE_NULL) continue;
            if (childNode->kind != NODE_MULTIPLE && childNode->kind != NODE_BLOCK) {
                for (int32_t i = 0; i < indentDepth * 4; i++) fprintf(file, " ");
            }
            node_print(file, childNode);
            if (childNode->kind != NODE_MULTIPLE && childNode->kind != NODE_BLOCK && childNode->kind != NODE_IF &&
                childNode->kind != NODE_WHILE) {
                fprintf(file, ";\n");
            }
        }
        indentDepth--;
        for (int32_t i = 0; i < indentDepth * 4; i++) fprintf(file, " ");
        fprintf(file, "}\n");
    }

    if (node->kind == NODE_IF) {
        fprintf(file, "if (");
        node_print(file, node->condition);
        fprintf(file, ")\n");
        node_print(file, node->thenBlock);
        if (node->elseBlock != NULL) {
            for (int32_t i = 0; i < indentDepth * 4; i++) fprintf(file, " ");
            fprintf(file, "else\n");
            node_print(file, node->elseBlock);
        }
    }

    if (node->kind == NODE_WHILE) {
        fprintf(file, "while (");
        if (node->condition != NULL) {
            node_print(file, node->condition);
        } else {
            fprintf(file, "1");
        }
        fprintf(file, ")\n");
        node_print(file, node->thenBlock);
    }

    if (node->kind == NODE_RETURN) {
        fprintf(file, "return ");
        node_print(file, node->unary);
    }

    if (node->kind == NODE_INTEGER) {
        fprintf(file, "%lld", node->integer);
    }
    if (node->kind == NODE_VARIABLE) {
        fprintf(file, "%s", node->local->name);
    }
    if (node->kind == NODE_FUNCCALL) {
        fprintf(file, "%s(", node->functionName);
        for (size_t i = 0; i < node->nodes->size; i++) {
            node_print(file, list_get(node->nodes, i));
            if (i != node->nodes->size - 1) fprintf(file, ", ");
        }
        fprintf(file, ")");
    }

    if (node->kind >= NODE_NEG && node->kind <= NODE_LOGIC_NOT) {
        bool hasTypeChanged = memcmp(node->type, node->lhs->type, sizeof(Type));
        if (hasTypeChanged) {
            fprintf(file, "[");
            type_print(file, node->type);
            fprintf(file, "]");
        }
        fprintf(file, "(");
        if (node->kind == NODE_NEG) fprintf(file, "- ");
        if (node->kind == NODE_ADDR) fprintf(file, "& ");
        if (node->kind == NODE_DEREF) fprintf(file, "* ");
        if (node->kind == NODE_LOGIC_NOT) fprintf(file, "! ");
        if (hasTypeChanged) {
            fprintf(file, "[");
            type_print(file, node->unary->type);
            fprintf(file, "]");
        }
        node_print(file, node->unary);
        fprintf(file, ")");
    }

    if (node->kind >= NODE_ASSIGN && node->kind <= NODE_LOGIC_OR) {
        bool hasTypeChanged = memcmp(node->type, node->lhs->type, sizeof(Type)) || node->kind == NODE_ASSIGN;
        if (hasTypeChanged) {
            fprintf(file, "[");
            type_print(file, node->type);
            fprintf(file, "]");
        }
        fprintf(file, "(");
        if (hasTypeChanged) {
            fprintf(file, "[");
            type_print(file, node->lhs->type);
            fprintf(file, "]");
        }
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
        if (hasTypeChanged) {
            fprintf(file, "[");
            type_print(file, node->rhs->type);
            fprintf(file, "]");
        }
        node_print(file, node->rhs);
        fprintf(file, ")");
    }
}
