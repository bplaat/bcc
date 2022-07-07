#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <stdlib.h>

#include "utils.h"

typedef enum TokenType {
    TOKEN_TYPE_EOF,
    TOKEN_TYPE_LPAREN,
    TOKEN_TYPE_RPAREN,
    TOKEN_TYPE_LCURLY,
    TOKEN_TYPE_RCURLY,
    TOKEN_TYPE_SEMICOLON,

    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_VARIABLE,

    TOKEN_TYPE_IF,
    TOKEN_TYPE_ELSE,
    TOKEN_TYPE_WHILE,
    TOKEN_TYPE_FOR,
    TOKEN_TYPE_RETURN,

    TOKEN_TYPE_ASSIGN,
    TOKEN_TYPE_ADD,
    TOKEN_TYPE_SUB,
    TOKEN_TYPE_STAR,
    TOKEN_TYPE_DIV,
    TOKEN_TYPE_MOD,
    TOKEN_TYPE_EQ,
    TOKEN_TYPE_NEQ,
    TOKEN_TYPE_LT,
    TOKEN_TYPE_LTEQ,
    TOKEN_TYPE_GT,
    TOKEN_TYPE_GTEQ,
    TOKEN_TYPE_ADDR,
    TOKEN_TYPE_LOGIC_NOT,
    TOKEN_TYPE_LOGIC_AND,
    TOKEN_TYPE_LOGIC_OR
} TokenType;

typedef struct Token {
    TokenType type;
    union {
        int64_t number;
        char *string;
    };
    int32_t position;
} Token;

Token *token_new(TokenType type);

void token_type_to_string(TokenType type, char *buffer);

List *lexer(char *text);

#endif
