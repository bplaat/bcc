#ifndef NODE_H

#include <stdio.h>

#include "type.h"
#include "utils.h"

typedef struct Local {
    char *name;
    Type *type;
    size_t offset;
} Local;

typedef enum NodeKind {
    NODE_NULL,
    NODE_PROGRAM,
    NODE_FUNCDEF,
    NODE_FUNCARG,
    NODE_MULTIPLE,
    NODE_BLOCK,

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

typedef struct Node Node;
struct Node {
    NodeKind kind;
    Type *type;
    union {
        // Integer kind
        int64_t integer;

        // Variable kind
        Local *local;

        // Unary kind
        Node *unary;

        // Operation kind
        struct {
            Node *lhs;
            Node *rhs;
        };

        // Program kind
        List *funcs;

        // Funcdef, funccall, multiple and block kind
        struct {
            union {
                char *functionName;
                Node *parentBlock;
            };
            List *nodes;
            List *locals;
            size_t argsSize;
        };

        // If and while kind
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
