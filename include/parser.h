#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stdio.h>

#include "lexer.h"
#include "list.h"

// Type
typedef enum TypeKind {
    TYPE_INTEGER,
    TYPE_POINTER,
    TYPE_ARRAY,
} TypeKind;

typedef struct Type Type;
struct Type {
    TypeKind kind;
    size_t size;
    Type *base;
};

Type *type_new(TypeKind kind, size_t size);

Type *type_new_pointer(Type *base);

Type *type_new_array(Type *base, size_t size);

void type_dump(FILE *f, Type *type);

// Local
typedef struct Local {
    char *name;
    Type *type;
    size_t offset;
} Local;

// Node
typedef enum NodeKind {
    NODE_FUNCTION,
    NODE_NODES,

    NODE_LOCAL,
    NODE_INTEGER,

    NODE_TENARY,
    NODE_IF,
    NODE_WHILE,
    NODE_DOWHILE,
    NODE_RETURN,

    NODE_UNARY_BEGIN,
    NODE_NEG,
    NODE_NOT,
    NODE_LOGICAL_NOT,
    NODE_ADDR,
    NODE_DEREF,
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
            List locals;
            size_t locals_size;
        };

        // Tenary, If, while, dowhile
        struct {
            Node *condition;
            Node *then_block;
            Node *else_block;
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

Node *node_new_integer(Token *token, int64_t integer);

Node *node_new_unary(NodeKind kind, Token *token, Node *unary);

Node *node_new_operation(NodeKind kind, Token *token, Node *lhs, Node *rhs);

Node *node_new_nodes(NodeKind kind, Token *token);

Node *node_new_function(NodeKind kind, Token *token);

Local *node_find_local(Node *node, char *name);

void node_dump(FILE *f, Node *node, int32_t indent);

// Parser
typedef struct Parser {
    char *text;
    Token *tokens;
    size_t tokens_size;
    size_t position;
    Node *current_function;
} Parser;

Node *parser(char *text, Token *tokens, size_t tokens_size);

void parser_eat(Parser *parser, TokenKind token_kind);

Type *parser_type(Parser *parser);
Type *parser_type_suffix(Parser *parser, Type *type);

Node *parser_add_node(Parser *parser, Token *token, Node *lhs, Node *rhs);
Node *parser_sub_node(Parser *parser, Token *token, Node *lhs, Node *rhs);
Node *parser_mul_node(Parser *parser, Token *token, Node *lhs, Node *rhs);
Node *parser_div_node(Parser *parser, Token *token, Node *lhs, Node *rhs);

Node *parser_function(Parser *parser);
Node *parser_block(Parser *parser);
Node *parser_statement(Parser *parser);
Node *parser_declarations(Parser *parser);
Node *parser_assigns(Parser *parser);
Node *parser_assign(Parser *parser);
Node *parser_tenary(Parser *parser);
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
