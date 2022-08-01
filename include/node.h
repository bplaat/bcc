#ifndef NODE_H

#include "type.h"
#include "utils.h"

typedef struct Local {
    char *name;
    Type *type;
    size_t offset;
} Local;

Local *local_new(char *string, Type *type, size_t offset);

typedef enum NodeKind {
    NODE_PROGRAM,
    NODE_FUNCDEF,
    NODE_BLOCK,

    NODE_INTEGER,
    NODE_LOCAL,
    NODE_FUNCCALL,

    NODE_IF,
    NODE_WHILE,
    NODE_RETURN,

    NODE_NEG,
    NODE_LOGIC_NOT,
    NODE_REF,
    NODE_DEREF,

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
    NODE_LOGIC_OR,
    NODE_LOGIC_AND
} NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind kind;
    Type *type;
    union {
        // Integer kind
        int64_t integer;

        // Local kind
        Local *local;

        // Unary kind
        Node *unary;

        // Operation kind
        struct {
            Node *lhs;
            Node *rhs;
        };

        // Program, funcdef, funccall and block kind
        struct {
            char *funcname;
            List *nodes;
            List *locals;
            size_t argsSize;
            bool isLeaf;
        };

        // If and while kind
        struct {
            Node *condition;
            Node *thenBlock;
            Node *elseBlock;
        };
    };
};

Node *node_new(NodeKind kind);

Node *node_new_integer(int64_t integer);

Node *node_new_local(Local *local);

Node *node_new_unary(NodeKind kind, Node *unary);

Node *node_new_operation(NodeKind kind, Node *lhs, Node *rhs);

Node *node_new_multiple(NodeKind kind);

char *node_to_string(Node *node);

#endif
