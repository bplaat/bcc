#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdint.h>

#include "lexer.h"

// Node
typedef enum NodeType {
    NODE_INTEGER,

    NODE_OPERATION_BEGIN,
    NODE_ADD,
    NODE_SUB,
    NODE_OPERATION_END,
} NodeType;

typedef struct Node Node;
struct Node {
    NodeType type;
    union {
        int64_t integer;
        struct {
            Node *lhs;
            Node *rhs;
        };
    };
};

Node *node_new(NodeType type);

Node *node_new_operation(NodeType type, Node *lhs, Node *rhs);

void node_dump(FILE *f, Node *node);

// Parser
Node *parser_add(Lexer *lexer);
Node *parser_primary(Lexer *lexer);

#endif
