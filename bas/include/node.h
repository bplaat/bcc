#pragma once

#include <stdint.h>
#include "token.h"
#include "list.h"

typedef enum NodeKind {
    NODE_PROGRAM,

    NODE_LABEL,
    NODE_INSTRUCTION,
    NODE_TIMES,

    NODE_INTEGER,
    NODE_STRING,
    NODE_KEYWORD,
    NODE_ADDR,

    NODE_NEG,
    NODE_ADD,
    NODE_SUB,
    NODE_MUL,
    NODE_DIV,
    NODE_MOD,

    NODE_REGISTER
} NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind kind;
    union {
        int64_t integer;
        char *string;
        struct {
            int32_t reg;
            int32_t size;
            Node *unary;
        };
        struct {
            Node *lhs;
            Node *rhs;
        };
        struct {
            TokenKind opcode;
            List *nodes;
        };
    };
};

Node *node_new(NodeKind kind);

Node *node_new_integer(int64_t integer);

Node *node_new_string(NodeKind kind, char *string);

Node *node_new_unary(NodeKind kind, Node *unary);

Node *node_new_operation(NodeKind kind, Node *lhs, Node *rhs);

Node *node_new_multiple(NodeKind kind);

Node *node_new_register(int32_t reg, int32_t size);

char *node_to_string(Node *node);
