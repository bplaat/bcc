#include "node.h"

#include <stdlib.h>
#include <string.h>

Var *var_new(VarKind kind, char *name, Type *type) {
    Var *var = malloc(sizeof(Var));
    var->kind = kind;
    var->name = name;
    var->type = type;
    return var;
}

Var *var_new_local(char *name, Type *type, size_t offset) {
    Var *var = var_new(VAR_LOCAL, name, type);
    var->offset = offset;
    return var;
}

Var *var_new_global(char *name, Type *type, uint8_t *data) {
    Var *var = var_new(VAR_GLOBAL, name, type);
    var->data = data;
    return var;
}

Node *node_new(NodeKind kind) {
    Node *node = malloc(sizeof(Node));
    node->kind = kind;
    return node;
}

Node *node_new_integer(int64_t integer, int32_t size, bool isUnsigned) {
    Node *node = node_new(NODE_INTEGER);
    node->type = type_new(TYPE_INTEGER, size, isUnsigned);
    node->integer = integer;
    return node;
}

Node *node_new_var(Var *var) {
    Node *node = node_new(NODE_VAR);
    node->type = var->type;
    node->var = var;
    return node;
}

Node *node_new_unary(NodeKind kind, Node *unary) {
    Node *node = node_new(kind);
    node->type = unary->type;
    node->unary = unary;
    return node;
}

Node *node_new_operation(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = node_new(kind);
    if (lhs->type->kind == TYPE_ARRAY) {
        node->type = type_new_pointer(lhs->type->base);
    } else if (rhs->type->size > lhs->type->size) {
        node->type = rhs->type;
    } else {
        node->type = lhs->type;
    }
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *node_new_multiple(NodeKind kind) {
    Node *node = node_new(kind);
    node->nodes = list_new(8);
    return node;
}

char *node_to_string(Node *node) {
    if (node->kind == NODE_PROGRAM) {
        List *sb = list_new(8);
        if (node->vars->size > 0) {
            for (size_t i = 0; i < node->vars->size; i++) {
                Var *var = list_get(node->vars, i);
                list_add(sb, type_to_string(var->type));
                list_add(sb, " ");
                list_add(sb, var->name);
                if (var->data != NULL) {
                    list_add(sb, " = { ");
                    for (int32_t i = 0; i < var->type->size; i++) {
                        list_add(sb, format("0x%02x", var->data[i]));
                        if (i != var->type->size - 1) {
                            list_add(sb, ", ");
                        }
                    }
                    list_add(sb, " }");
                }
                list_add(sb, "; ");
            }
            list_add(sb, "\n");
        }
        for (size_t i = 0; i < node->nodes->size; i++) {
            list_add(sb, node_to_string(list_get(node->nodes, i)));
            list_add(sb, "\n");
        }
        return list_to_string(sb);
    }
    if (node->kind == NODE_FUNCDEF) {
        List *sb = list_new(8);
        list_add(sb, type_to_string(node->type));
        list_add(sb, " ");
        list_add(sb, node->funcname);
        list_add(sb, "(");
        if (node->argsSize > 0) {
            for (size_t i = 0; i < node->argsSize; i++) {
                Var *var = list_get(node->vars, i);
                list_add(sb, type_to_string(var->type));
                list_add(sb, " ");
                list_add(sb, var->name);
                if (i != node->argsSize - 1) list_add(sb, ", ");
            }
        }
        list_add(sb, ") { ");
        for (size_t i = node->argsSize; i < node->vars->size; i++) {
            Var *var = list_get(node->vars, i);
            list_add(sb, type_to_string(var->type));
            list_add(sb, " ");
            list_add(sb, var->name);
            list_add(sb, "; ");
        }
        for (size_t i = 0; i < node->nodes->size; i++) {
            list_add(sb, node_to_string(list_get(node->nodes, i)));
            list_add(sb, "; ");
        }
        list_add(sb, "}");
        return list_to_string(sb);
    }
    if (node->kind == NODE_BLOCK) {
        List *sb = list_new(8);
        list_add(sb, "{ ");
        for (size_t i = 0; i < node->nodes->size; i++) {
            list_add(sb, node_to_string(list_get(node->nodes, i)));
            list_add(sb, "; ");
        }
        list_add(sb, "}");
        return list_to_string(sb);
    }

    if (node->kind == NODE_INTEGER) {
        return format("%lld", node->integer);
    }
    if (node->kind == NODE_VAR) {
        return strdup(node->var->name);
    }
    if (node->kind == NODE_FUNCCALL) {
        List *sb = list_new(8);
        list_add(sb, node->funcname);
        list_add(sb, "(");
        for (size_t i = 0; i < node->nodes->size; i++) {
            list_add(sb, node_to_string(list_get(node->nodes, i)));
            if (i != node->nodes->size - 1) list_add(sb, ", ");
        }
        list_add(sb, ")");
        return list_to_string(sb);
    }

    if (node->kind == NODE_IF) {
        List *sb = list_new(8);
        list_add(sb, "if (");
        list_add(sb, node_to_string(node->condition));
        list_add(sb, ") ");
        list_add(sb, node_to_string(node->thenBlock));
        if (node->elseBlock != NULL) {
            list_add(sb, "else ");
            list_add(sb, node_to_string(node->elseBlock));
        }
        return list_to_string(sb);
    }
    if (node->kind == NODE_WHILE) {
        List *sb = list_new(8);
        list_add(sb, "while (");
        list_add(sb, node->condition != NULL ? node_to_string(node->condition) : "1");
        list_add(sb, ") ");
        list_add(sb, node_to_string(node->thenBlock));
        return list_to_string(sb);
    }
    if (node->kind == NODE_RETURN) {
        List *sb = list_new(8);
        list_add(sb, "return ");
        list_add(sb, node_to_string(node->unary));
        return list_to_string(sb);
    }

    if (node->kind >= NODE_NEG && node->kind <= NODE_DEREF) {
        List *sb = list_new(8);
        if (memcmp(node->type, node->unary->type, sizeof(Type))) {
            list_add(sb, "[");
            list_add(sb, type_to_string(node->type));
            list_add(sb, "]");
        }
        if (node->kind == NODE_NEG) list_add(sb, "(- ");
        if (node->kind == NODE_LOGIC_NOT) list_add(sb, "(! ");
        if (node->kind == NODE_REF) list_add(sb, "(& ");
        if (node->kind == NODE_DEREF) list_add(sb, "(* ");
        list_add(sb, node_to_string(node->unary));
        list_add(sb, ")");
        return list_to_string(sb);
    }

    if (node->kind >= NODE_ASSIGN && node->kind <= NODE_LOGIC_AND) {
        List *sb = list_new(8);
        bool typeChanged = memcmp(node->type, node->rhs->type, sizeof(Type));
        if (node->kind != NODE_ASSIGN && node->kind != NODE_ASSIGN_PTR && typeChanged) {
            list_add(sb, "[");
            list_add(sb, type_to_string(node->type));
            list_add(sb, "]");
        }
        list_add(sb, "(");
        if (node->kind == NODE_ASSIGN_PTR) list_add(sb, "*(");
        list_add(sb, node_to_string(node->lhs));
        if (node->kind == NODE_ASSIGN_PTR) list_add(sb, ")");
        if (node->kind == NODE_ASSIGN || node->kind == NODE_ASSIGN_PTR) list_add(sb, " = ");
        if (node->kind == NODE_ADD) list_add(sb, " + ");
        if (node->kind == NODE_SUB) list_add(sb, " - ");
        if (node->kind == NODE_MUL) list_add(sb, " * ");
        if (node->kind == NODE_DIV) list_add(sb, " / ");
        if (node->kind == NODE_MOD) list_add(sb, " %% ");
        if (node->kind == NODE_EQ) list_add(sb, " == ");
        if (node->kind == NODE_NEQ) list_add(sb, " != ");
        if (node->kind == NODE_LT) list_add(sb, " < ");
        if (node->kind == NODE_LTEQ) list_add(sb, " <= ");
        if (node->kind == NODE_GT) list_add(sb, " > ");
        if (node->kind == NODE_GTEQ) list_add(sb, " >= ");
        if (node->kind == NODE_LOGIC_OR) list_add(sb, " || ");
        if (node->kind == NODE_LOGIC_AND) list_add(sb, " && ");
        if ((node->kind == NODE_ASSIGN || node->kind == NODE_ASSIGN_PTR) && typeChanged) {
            list_add(sb, "[");
            list_add(sb, type_to_string(node->type));
            list_add(sb, "]");
        }
        list_add(sb, node_to_string(node->rhs));
        list_add(sb, ")");
        return list_to_string(sb);
    }
    return NULL;
}
