#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <stdlib.h>

#include "utils.h"

typedef enum TokenKind {
    TOKEN_EOF,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LCURLY,
    TOKEN_RCURLY,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,

    TOKEN_INTEGER,
    TOKEN_VARIABLE,

    TOKEN_INT,
    TOKEN_LONG,
    TOKEN_SIGNED,
    TOKEN_UNSIGNED,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_RETURN,

    TOKEN_ASSIGN,
    TOKEN_ADD,
    TOKEN_SUB,
    TOKEN_STAR,
    TOKEN_DIV,
    TOKEN_MOD,
    TOKEN_EQ,
    TOKEN_NEQ,
    TOKEN_LT,
    TOKEN_LTEQ,
    TOKEN_GT,
    TOKEN_GTEQ,
    TOKEN_ADDR,
    TOKEN_LOGIC_NOT,
    TOKEN_LOGIC_AND,
    TOKEN_LOGIC_OR
} TokenKind;

typedef struct Token {
    TokenKind kind;
    int32_t position;
    union {
        int64_t integer;
        char *string;
    };
} Token;

Token *token_new(TokenKind kind);

char *token_to_string(TokenKind kind);

List *lexer(char *text);

#endif
