#pragma once

#include "object.h"
#include "node.h"

typedef struct Parser {
    Arch *arch;
    char *path;
    List *lines;
    List *tokens;
    int32_t position;
} Parser;

Node *parser(Arch *arch, char *path, List *lines, List *tokens);

void parser_eat(Parser *parser, TokenKind kind);

Node *parser_program(Parser *parser);
Node *parser_statement(Parser *parser);
Node *parser_add(Parser *parser);
Node *parser_mul(Parser *parser);
Node *parser_unary(Parser *parser);
Node *parser_primary(Parser *parser);
