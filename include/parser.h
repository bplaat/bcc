#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stdio.h>

#include "list.h"
#include "map.h"
#include "lexer.h"

// Local
typedef struct Local {
    char *name;
    size_t size;
    size_t address;
} Local;

// Node
typedef enum NodeType {
    NODE_BLOCK,
    NODE_NODES,

    NODE_LOCAL,
    NODE_INTEGER,

    NODE_UNARY_BEGIN,
    NODE_NEG,
    NODE_NOT,
    NODE_LOGICAL_NOT,
    NODE_UNARY_END,

    NODE_OPERATION_BEGIN,
    NODE_ASSIGN,
    NODE_ADD,
    NODE_SUB,
    NODE_MUL,
    NODE_DIV,
    NODE_MOD,
    NODE_AND,
    NODE_OR,
    NODE_XOR,
    NODE_SHL,
    NODE_SHR,

    NODE_COMPARE_BEGIN,
    NODE_EQ,
    NODE_NEQ,
    NODE_LT,
    NODE_LTEQ,
    NODE_GT,
    NODE_GTEQ,
    NODE_COMPARE_END,

    NODE_LOGICAL_AND,
    NODE_LOGICAL_OR,

    NODE_OPERATION_END,
} NodeType;

typedef struct Node Node;
struct Node {
    NodeType type;
    Token *token;
    union {
        struct {
            List nodes;
            Map locals;
        };
        Local *local;
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

Node *node_new_nodes(NodeType type, Token *token);

Node *node_new_block(NodeType type, Token *token);

void node_dump(FILE *f, Node *node);

// Parser
typedef struct Parser {
    Token *tokens;
    size_t tokens_size;
    size_t position;
    Node *currentBlockNode;
} Parser;

Node *parser(Token *tokens, size_t tokens_size);

void parser_eat(Parser *parser, TokenType type);

Node *parser_block(Parser *parser);
Node *parser_statement(Parser *parser);
Node *parser_assigns(Parser *parser);
Node *parser_assign(Parser *parser);
Node *parser_logical(Parser *parser);
Node *parser_equality(Parser *parser);
Node *parser_relational(Parser *parser);
Node *parser_bitwise(Parser *parser);
Node *parser_shift(Parser *parser);
Node *parser_add(Parser *parser);
Node *parser_mul(Parser *parser);
Node *parser_unary(Parser *parser);
Node *parser_primary(Parser *parser);

#endif
