#ifndef NODE_H

#include <stdio.h>

#include "type.h"
#include "utils.h"

typedef enum NodeKind {
    NODE_MULTIPLE,
    NODE_FUNCTION,
    NODE_FUNCARG,
    NODE_BLOCK,
    NODE_NULL,

    NODE_VARIABLE,
    NODE_INTEGER,
    NODE_FUNCCALL,

    NODE_IF,
    NODE_WHILE,
    NODE_RETURN,

    NODE_NEG,
    NODE_ADDR,
    NODE_DEREF,
    NODE_LOGIC_NOT,

    NODE_ASSIGN,
    NODE_ASSIGN_PTR,
    NODE_ADD,
    NODE_SUB,
    NODE_MUL,
    NODE_DIV,
    NODE_MOD,
    NODE_EQ,
    NODE_NEQ,
    NODE_LT,
    NODE_LTEQ,
    NODE_GT,
    NODE_GTEQ,
    NODE_LOGIC_AND,
    NODE_LOGIC_OR
} NodeKind;

typedef struct Local {
    char *name;
    Type *type;
    size_t offset;
} Local;

typedef struct Node Node;
struct Node {
    NodeKind kind;
    Type *type;
    union {
        int64_t integer;
        Local *local;

        Node *unary;
        struct {
            Node *lhs;
            Node *rhs;
        };
        struct {
            union {
                char *functionName;
                Node *parentBlock;
            };
            List *nodes;
            List *locals;
            size_t argsSize;
        };
        struct {
            Node *condition;
            Node *thenBlock;
            Node *elseBlock;
        };
    };
};

Local *local_new(char *string, Type *type);

Node *node_new(NodeKind kind);

Node *node_new_integer(int64_t integer);

Node *node_new_unary(NodeKind kind, Node *unary);

Node *node_new_operation(NodeKind kind, Node *lhs, Node *rhs);

Node *node_new_multiple(NodeKind kind);

Local *node_find_local(Node *block, char *name);

void node_print(FILE *file, Node *node);

#endif
