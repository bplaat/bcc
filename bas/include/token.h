#pragma once

#include <stdint.h>

typedef enum TokenKind {
    TOKEN_INTEGER,
    TOKEN_STRING,
    TOKEN_KEYWORD,

    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_NEWLINE,
    TOKEN_EOF,

    TOKEN_ADD,
    TOKEN_SUB,
    TOKEN_MUL,
    TOKEN_DIV,
    TOKEN_MOD,

    // Shared
    TOKEN_SECTION,
    TOKEN_TIMES,
    TOKEN_DB,
    TOKEN_DW,
    TOKEN_DD,
    TOKEN_DQ,

    // x86_64
    TOKEN_BYTE,
    TOKEN_WORD,
    TOKEN_DWORD,
    TOKEN_QWORD,
    TOKEN_PTR,

    TOKEN_EAX,
    TOKEN_ECX,
    TOKEN_EDX,
    TOKEN_EBX,
    TOKEN_ESP,
    TOKEN_EBP,
    TOKEN_ESI,
    TOKEN_EDI,
    TOKEN_RAX,
    TOKEN_RCX,
    TOKEN_RDX,
    TOKEN_RBX,
    TOKEN_RSP,
    TOKEN_RBP,
    TOKEN_RSI,
    TOKEN_RDI,

    TOKEN_X86_64_NOP,
    TOKEN_X86_64_MOV,
    TOKEN_X86_64_LEA,
    TOKEN_X86_64_ADD,
    TOKEN_X86_64_ADC,
    TOKEN_X86_64_SUB,
    TOKEN_X86_64_SBB,
    TOKEN_X86_64_CMP,
    TOKEN_X86_64_AND,
    TOKEN_X86_64_OR,
    TOKEN_X86_64_XOR,
    TOKEN_X86_64_NEG,
    TOKEN_X86_64_PUSH,
    TOKEN_X86_64_POP,
    TOKEN_X86_64_SYSCALL,
    TOKEN_X86_64_CDQ,
    TOKEN_X86_64_CQO,
    TOKEN_X86_64_LEAVE,
    TOKEN_X86_64_RET
} TokenKind;

typedef struct Token {
    TokenKind kind;
    int32_t line;
    int32_t position;
    union {
        int64_t integer;
        char *string;
    };
} Token;

Token *token_new(TokenKind kind, int32_t line, int32_t position);

Token *token_new_integer(TokenKind kind, int32_t line, int32_t position, int64_t integer);

Token *token_new_string(TokenKind kind, int32_t line, int32_t position, char *string);

char *token_kind_to_string(TokenKind kind);
