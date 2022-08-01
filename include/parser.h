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
    Node *currentFuncdef;
} Parser;

Node *parser(Arch *arch, char *text, List *tokens);

void parser_eat(Parser *parser, TokenKind kind);

Type *parser_type(Parser *parser);

Local *parser_find_local(Parser *parser, char *name);

Node *parser_program(Parser *parser);
Node *parser_block(Parser *parser);
Node *parser_statement(Parser *parser);
Node *parser_decls(Parser *parser);
Node *parser_assigns(Parser *parser, Type *declType);
Node *parser_assign(Parser *parser, Type *declType);
Node *parser_logic(Parser *parser);
Node *parser_equality(Parser *parser);
Node *parser_relational(Parser *parser);
Node *parser_add(Parser *parser);
Node *parser_mul(Parser *parser);
Node *parser_unary(Parser *parser);
Node *parser_primary(Parser *parser);

#endif
