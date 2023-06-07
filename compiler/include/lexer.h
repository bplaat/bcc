#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <stddef.h>

// Source
typedef struct Source {
    char *path;
    char *basename;
    char *dirname;
    char *text;
} Source;

Source *source_new(char *path, char *text);

// Token
typedef enum TokenKind {
    TOKEN_EOF,

    TOKEN_INTEGER_BEGIN,
    TOKEN_I8,
    TOKEN_I32,
    TOKEN_I64,
    TOKEN_U32,
    TOKEN_U64,
    TOKEN_INTEGER_END,

    TOKEN_VARIABLE,
    TOKEN_STRING,

    TOKEN_TYPE_BEGIN,
    TOKEN_CHAR,
    TOKEN_SHORT,
    TOKEN_INT,
    TOKEN_LONG,
    TOKEN_SIGNED,
    TOKEN_UNSIGNED,
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
    TOKEN_LBLOCK,
    TOKEN_RBLOCK,
    TOKEN_LCURLY,
    TOKEN_RCURLY,
    TOKEN_COMMA,
    TOKEN_QUESTION,
    TOKEN_COLON,
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
    Source *source;
    int32_t line;
    int32_t column;
    union {
        int64_t integer;
        char *string;
    };
} Token;

char *token_kind_to_string(TokenKind kind);

// Lexer
typedef struct Keyword {
    char *keyword;
    TokenKind kind;
} Keyword;

Token *lexer(char *path, char *text, size_t *tokens_size);

#endif
