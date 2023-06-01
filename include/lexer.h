#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <stddef.h>

// Token
typedef enum TokenType {
    TOKEN_EOF,
    TOKEN_UNKNOWN,
    TOKEN_INTEGER,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LCURLY,
    TOKEN_RCURLY,
    TOKEN_SEMICOLON,
    TOKEN_NOT,
    TOKEN_LOGICAL_NOT,
    TOKEN_ADD,
    TOKEN_SUB,
    TOKEN_MUL,
    TOKEN_DIV,
    TOKEN_MOD,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_XOR,
    TOKEN_SHL,
    TOKEN_SHR,
    TOKEN_EQ,
    TOKEN_NEQ,
    TOKEN_LT,
    TOKEN_LTEQ,
    TOKEN_GT,
    TOKEN_GTEQ,
    TOKEN_LOGICAL_AND,
    TOKEN_LOGICAL_OR,
} TokenType;

typedef struct Token {
    TokenType type;
    int32_t line;
    int32_t column;
    union {
        char character;
        int64_t integer;
    };
} Token;

// Lexer
Token *lexer(char *text, size_t *tokens_size);

#endif
