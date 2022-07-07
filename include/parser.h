#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

#include "lexer.h"
#include "type.h"
#include "utils.h"

typedef enum NodeKind {
    NODE_BLOCK,
    NODE_NULL,

    NODE_VARIABLE,
    NODE_NUMBER,

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

typedef struct Local {
    char *name;
    size_t size;
    size_t offset;
} Local;

typedef struct Node Node;
struct Node {
    NodeKind kind;
    Type *type;
    Token *token;
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
};

Node *node_new(NodeKind kind);

Node *node_new_number(int64_t number);

Node *node_new_string(char *string);

Node *node_new_unary(NodeKind kind, Node *unary);

Node *node_new_operation(NodeKind kind, Node *lhs, Node *rhs);

void node_print(FILE *file, Node *node);

Node *parser(char *text, List *tokens);

void parser_eat(TokenKind kind);

Local *block_find_local(Node *block, char *name);

void block_create_locals(Node *node);

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

#endif
