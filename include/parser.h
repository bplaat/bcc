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
    bool is_signed;
    size_t size;
    Type *base;
};

Type *type_new(TypeKind kind, size_t size);

Type *type_new_integer(size_t size, bool is_signed);

Type *type_new_pointer(Type *base);

Type *type_new_array(Type *base, size_t size);

void type_dump(FILE *f, Type *type);

// Function
typedef struct Argument {
    char *name;
    Type *type;
} Argument;

typedef struct Function {
    char *name;
    Type *return_type;
    List arguments;
    bool is_leaf;
    uint8_t *address;
} Function;

// Local
typedef struct Local {
    char *name;
    Type *type;
    size_t offset;
} Local;

// Node
typedef enum NodeKind {
    NODE_PROGRAM,
    NODE_FUNCTION,
    NODE_NODES,

    NODE_LOCAL,
    NODE_INTEGER,
    NODE_CALL,

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
        // Program, function, nodes, call
        struct {
            Function *function;
            List nodes;
            List functions;
            List locals;
            size_t locals_size;
        };

        // Tenary, if, while, dowhile
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

Function *node_find_function(Node *node, char *name);

Local *node_find_local(Node *node, char *name);

void node_dump(FILE *f, Node *node, int32_t indent);

// Parser
typedef struct Parser {
    char *text;
    Token *tokens;
    size_t tokens_size;
    size_t position;
    Node *program;
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
Node *parser_deref_node(Parser *parser, Token *token, Node *unary);

Node *parser_program(Parser *parser);
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
Node *parser_postfix(Parser *parser);
Node *parser_primary(Parser *parser);

#endif
