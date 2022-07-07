#ifndef PARSER_H
#define PARSER_H

#include "node.h"
#include "lexer.h"
#include "utils.h"

Node *parser(char *text, List *tokens);

void parser_eat(TokenKind kind);

Node *parser_block(void);

Node *parser_statement(void);

Type *parser_type(void);

Node *parser_declarations(void);

Node *parser_assigns(void);

Node *parser_assign(void);

Node *parser_logic(void);

Node *parser_equality(void);

Node *parser_relational(void);

Node *parser_add(void);

Node *parser_mul(void);

Node *parser_unary(void);

Node *parser_primary(void);

#endif
