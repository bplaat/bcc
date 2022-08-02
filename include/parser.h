#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "node.h"
#include "utils.h"

typedef struct Parser {
    Arch *arch;
    char *text;
    List *tokens;
    int32_t position;
    Node *currentProgram;
    Node *currentFuncdef;
    int32_t uniqueGlobal;
} Parser;

Node *parser(Arch *arch, char *text, List *tokens);

void parser_eat(Parser *parser, TokenKind kind);

Type *parser_type(Parser *parser);

Type *parser_type_suffix(Parser *parser, Type *type);

Var *parser_find_var(Parser *parser, char *name);

Node *parser_program(Parser *parser);
Node *parser_block(Parser *parser);
Node *parser_statement(Parser *parser);
Node *parser_decls(Parser *parser);
Node *parser_assigns(Parser *parser);
Node *parser_assign(Parser *parser);
Node *parser_logic(Parser *parser);
Node *parser_equality(Parser *parser);
Node *parser_relational(Parser *parser);
Node *parser_add(Parser *parser);
Node *parser_mul(Parser *parser);
Node *parser_unary(Parser *parser);
Node *parser_primary_suffix(Parser *parser);
Node *parser_primary(Parser *parser);

#endif
