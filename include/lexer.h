#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <stddef.h>

// Token
typedef enum TokenKind {
    TOKEN_EOF,
    TOKEN_UNKNOWN,

    TOKEN_VARIABLE,
    TOKEN_INTEGER,

    TOKEN_TYPE_BEGIN,
    TOKEN_INT,
    TOKEN_TYPE_END,

    TOKEN_SIZEOF,
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

    TOKEN_ASSIGN_BEGIN,
    TOKEN_ASSIGN,
    TOKEN_ADD_ASSIGN,
    TOKEN_SUB_ASSIGN,
    TOKEN_MUL_ASSIGN,
    TOKEN_DIV_ASSIGN,
    TOKEN_MOD_ASSIGN,
    TOKEN_AND_ASSIGN,
    TOKEN_OR_ASSIGN,
    TOKEN_XOR_ASSIGN,
    TOKEN_SHL_ASSIGN,
    TOKEN_SHR_ASSIGN,
    TOKEN_ASSIGN_END,

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
} TokenKind;

typedef struct Token {
    TokenKind kind;
    int32_t line;
    int32_t column;
    union {
        char character;
        int64_t integer;
        char *variable;
    };
} Token;

char *token_kind_to_string(TokenKind kind);

// Lexer
typedef struct Keyword {
    char *keyword;
    TokenKind kind;
} Keyword;

Token *lexer(char *text, size_t *tokens_size);

#endif
