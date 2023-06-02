#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stdio.h>

#include "lexer.h"
#include "list.h"
#include "map.h"

// Type
typedef enum TypeKind {
    TYPE_INTEGER,
} TypeKind;

typedef struct Type {
    TypeKind kind;
    size_t size;
} Type;

Type *type_new(TypeKind kind, size_t size);

// Local
typedef struct Local {
    char *name;
    Type *type;
    size_t address;
} Local;

// Node
typedef enum NodeKind {
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
} NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind kind;
    Token *token;
    Type *type;
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

Node *node_new(NodeKind kind, Token *token);

Node *node_new_unary(NodeKind kind, Token *token, Node *unary);

Node *node_new_operation(NodeKind kind, Token *token, Node *lhs, Node *rhs);

Node *node_new_nodes(NodeKind kind, Token *token);

Node *node_new_function(NodeKind kind, Token *token);

void node_dump(FILE *f, Node *node, int32_t indent);

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

void parser_eat(Parser *parser, TokenKind token_kind);

Node *parser_function(Parser *parser);
Node *parser_block(Parser *parser);
Node *parser_statement(Parser *parser);
Node *parser_declarations(Parser *parser);
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
