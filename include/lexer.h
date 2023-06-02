#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <stddef.h>

// Token
typedef enum TokenType {
    TOKEN_EOF,
    TOKEN_UNKNOWN,

    TOKEN_VARIABLE,
    TOKEN_INTEGER,

    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_DO,
    TOKEN_FOR,
    TOKEN_RETURN,

    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LCURLY,
    TOKEN_RCURLY,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,

    TOKEN_NOT,
    TOKEN_LOGICAL_NOT,

    TOKEN_ASSIGN,
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
        char *variable;
    };
} Token;

char *token_type_to_string(TokenType type);

// Lexer
typedef struct Keyword {
    char *keyword;
    TokenType type;
} Keyword;

Token *lexer(char *text, size_t *tokens_size);

#endif
