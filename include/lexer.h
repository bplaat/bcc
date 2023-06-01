#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

// Token
typedef enum TokenType {
    TOKEN_EOF,
    TOKEN_UNKNOWN,
    TOKEN_INTEGER,
    TOKEN_ADD,
    TOKEN_SUB,
} TokenType;

typedef struct Token {
    TokenType type;
    union {
        char character;
        int64_t integer;
    };
} Token;

// Lexer
typedef struct Lexer {
    char *text;
    char *c;
} Lexer;

Token lexer_next_token(Lexer *lexer);

#endif
