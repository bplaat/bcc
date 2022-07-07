#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdio.h>
#include "utils.h"

typedef enum NodeType {
    NODE_TYPE_BLOCK,
    NODE_TYPE_NULL,

    NODE_TYPE_VARIABLE,
    NODE_TYPE_NUMBER,

    NODE_TYPE_IF,
    NODE_TYPE_WHILE,
    NODE_TYPE_RETURN,

    NODE_TYPE_NEG,
    NODE_TYPE_ADDR,
    NODE_TYPE_DEREF,
    NODE_TYPE_LOGIC_NOT,

    NODE_TYPE_ASSIGN,
    NODE_TYPE_ADD,
    NODE_TYPE_SUB,
    NODE_TYPE_MUL,
    NODE_TYPE_DIV,
    NODE_TYPE_MOD,
    NODE_TYPE_EQ,
    NODE_TYPE_NEQ,
    NODE_TYPE_LT,
    NODE_TYPE_LTEQ,
    NODE_TYPE_GT,
    NODE_TYPE_GTEQ,
    NODE_TYPE_LOGIC_AND,
    NODE_TYPE_LOGIC_OR
} NodeType;

typedef struct Local {
    char *name;
    size_t size;
    size_t offset;
} Local;

typedef struct Node Node;
struct Node {
    NodeType type;
    union {
        int64_t number;
        char *string;

        Node *unary;
        struct {
            Node *lhs;
            Node *rhs;
        };
        struct {
            Node *parentBlock;
            List *statements;
            List *locals;
        };
        struct {
            Node *condition;
            Node *thenBlock;
            Node *elseBlock;
        };
    };
    Token *token;
};

extern char *text;
extern Token *token;

void parser_eat(TokenType type);

Node *parser_block(void);

Node *parser_statement(void);

Node *parser_assign(void);

Node *parser_logic(void);

Node *parser_equality(void);

Node *parser_relational(void);

Node *parser_add(void);

Node *parser_mul(void);

Node *parser_unary(void);

Node *parser_primary(void);

Local *block_find_local(Node *block, char *name);

void node_print(FILE *file, Node *node);

#endif
