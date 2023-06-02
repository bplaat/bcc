#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stdio.h>

#include "lexer.h"
#include "list.h"
#include "map.h"

// Local
typedef struct Local {
    char *name;
    size_t size;
    size_t address;
} Local;

// Node
typedef enum NodeType {
    NODE_FUNCTION,
    NODE_NODES,

    NODE_LOCAL,
    NODE_INTEGER,

    NODE_IF,
    NODE_WHILE,
    NODE_DOWHILE,
    NODE_RETURN,

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
        // Function, nodes
        struct {
            List nodes;
            Map locals;
            size_t locals_size;
        };

        // If, while, dowhile
        struct {
            Node *condition;
            Node *thenBlock;
            Node *elseBlock;
        };

        // Local
        Local *local;

        // Integer
        int64_t integer;

        // Unary, return
        Node *unary;

        // Operation
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

Node *node_new_function(NodeType type, Token *token);

void node_dump(FILE *f, Node *node);

// Parser
void print_error(char *text, Token *token, char *fmt, ...);

typedef struct Parser {
    char *text;
    Token *tokens;
    size_t tokens_size;
    size_t position;
    Node *currentFunction;
} Parser;

Node *parser(char *text, Token *tokens, size_t tokens_size);

void parser_eat(Parser *parser, TokenType type);

Node *parser_function(Parser *parser);
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
