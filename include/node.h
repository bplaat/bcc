#ifndef NODE_H

#include "type.h"
#include "utils.h"

typedef enum VarKind {
    VAR_LOCAL,
    VAR_GLOBAL
} VarKind;

typedef struct Var {
    VarKind kind;
    char *name;
    Type *type;
    union {
        size_t offset;
        uint8_t *data;
    };
} Var;

Var *var_new(VarKind kind, char *name, Type *type);

Var *var_new_local(char *name, Type *type, size_t offset);

Var *var_new_global(char *name, Type *type, uint8_t *data);

typedef enum NodeKind {
    NODE_PROGRAM,
    NODE_FUNCDEF,
    NODE_BLOCK,

    NODE_INTEGER,
    NODE_VAR,
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

        // Var kind
        Var *var;

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
            List *vars;
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

Node *node_new_integer(int64_t integer, int32_t size, bool isUnsigned);

Node *node_new_var(Var *var);

Node *node_new_unary(NodeKind kind, Node *unary);

Node *node_new_operation(NodeKind kind, Node *lhs, Node *rhs);

Node *node_new_multiple(NodeKind kind);

char *node_to_string(Node *node);

#endif
