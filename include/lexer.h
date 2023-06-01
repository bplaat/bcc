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
    TOKEN_ADD,
    TOKEN_SUB,
    TOKEN_MUL,
    TOKEN_DIV,
    TOKEN_MOD,
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
