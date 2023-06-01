#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stdio.h>

#include "lexer.h"

// Node
typedef enum NodeType {
    NODE_INTEGER,

    NODE_UNARY_BEGIN,
    NODE_NEG,
    NODE_UNARY_END,

    NODE_OPERATION_BEGIN,
    NODE_ADD,
    NODE_SUB,
    NODE_MUL,
    NODE_DIV,
    NODE_MOD,

    NODE_COMPARE_BEGIN,
    NODE_EQ,
    NODE_NEQ,
    NODE_LT,
    NODE_LTEQ,
    NODE_GT,
    NODE_GTEQ,
    NODE_COMPARE_END,

    NODE_OPERATION_END,
} NodeType;

typedef struct Node Node;
struct Node {
    NodeType type;
    Token *token;
    union {
        int64_t integer;
        Node *unary;
        struct {
            Node *lhs;
            Node *rhs;
        };
    };
};

Node *node_new(NodeType type, Token *token);

Node *node_new_unary(NodeType type, Token *token, Node *unary);

Node *node_new_operation(NodeType type, Token *token, Node *lhs, Node *rhs);

void node_dump(FILE *f, Node *node);

// Parser
typedef struct Parser {
    Token *tokens;
    size_t tokens_size;
    size_t position;
} Parser;

Node *parser(Token *tokens, size_t tokens_size);

void parser_eat(Parser *parser, TokenType type);

Node *parser_equality(Parser *parser);
Node *parser_relational(Parser *parser);
Node *parser_add(Parser *parser);
Node *parser_mul(Parser *parser);
Node *parser_unary(Parser *parser);
Node *parser_primary(Parser *parser);

#endif
