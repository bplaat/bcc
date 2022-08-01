#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "utils.h"

typedef enum TokenKind {
    TOKEN_INTEGER,
    TOKEN_VARIABLE,

    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LCURLY,
    TOKEN_RCURLY,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_EOF,

    TOKEN_INT,
    TOKEN_LONG,
    TOKEN_SIGNED,
    TOKEN_UNSIGNED,
    TOKEN_SIZEOF,
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
    TOKEN_AND,
    TOKEN_EQ,
    TOKEN_NEQ,
    TOKEN_LT,
    TOKEN_LTEQ,
    TOKEN_GT,
    TOKEN_GTEQ,
    TOKEN_LOGIC_AND,
    TOKEN_LOGIC_OR,
    TOKEN_LOGIC_NOT
} TokenKind;

typedef struct Token {
    TokenKind kind;
    size_t position;
    union {
        int64_t integer;
        char *string;
    };
} Token;

Token *token_new(TokenKind kind, size_t position);

Token *token_new_integer(size_t position, int64_t integer);

Token *token_new_string(TokenKind kind, size_t position, char *string);

char *token_kind_to_string(TokenKind kind);

bool token_kind_is_type(TokenKind kind);

typedef struct Keyword {
    char *keyword;
    TokenKind kind;
} Keyword;

List *lexer(char *text);

#endif
