#include "token.h"
#include <stdlib.h>
#include "utils.h"

Token *token_new(TokenKind kind, int32_t line, int32_t position) {
    Token *token = malloc(sizeof(Token));
    token->kind = kind;
    token->line = line;
    token->position = position;
    return token;
}

Token *token_new_integer(TokenKind kind, int32_t line, int32_t position, int64_t integer) {
    Token *token = token_new(kind, line, position);
    token->integer = integer;
    return token;
}

Token *token_new_string(TokenKind kind, int32_t line, int32_t position, char *string) {
    Token *token = token_new(kind, line, position);
    token->string = string;
    return token;
}

char *token_kind_to_string(TokenKind kind) {
    if (kind == TOKEN_INTEGER) return strdup("integer");
    if (kind == TOKEN_STRING) return strdup("string");
    if (kind == TOKEN_KEYWORD) return strdup("keyword");

    if (kind == TOKEN_LPAREN) return strdup("(");
    if (kind == TOKEN_RPAREN) return strdup(")");
    if (kind == TOKEN_LBRACKET) return strdup("[");
    if (kind == TOKEN_RBRACKET) return strdup("]");
    if (kind == TOKEN_COLON) return strdup(":");
    if (kind == TOKEN_COMMA) return strdup(",");
    if (kind == TOKEN_NEWLINE) return strdup("\\n");
    if (kind == TOKEN_EOF) return strdup("EOF");

    if (kind == TOKEN_ADD) return strdup("+");
    if (kind == TOKEN_SUB) return strdup("-");
    if (kind == TOKEN_MUL) return strdup("*");
    if (kind == TOKEN_DIV) return strdup("/");
    if (kind == TOKEN_MOD) return strdup("%");

    // Shared
    if (kind == TOKEN_SECTION) return strdup("section");
    if (kind == TOKEN_TIMES) return strdup("times");
    if (kind == TOKEN_DB) return strdup("db");
    if (kind == TOKEN_DW) return strdup("dw");
    if (kind == TOKEN_DD) return strdup("dd");
    if (kind == TOKEN_DQ) return strdup("dq");

    // x86_64
    if (kind == TOKEN_BYTE) return strdup("byte");
    if (kind == TOKEN_WORD) return strdup("word");
    if (kind == TOKEN_DWORD) return strdup("dword");
    if (kind == TOKEN_QWORD) return strdup("qword");
    if (kind == TOKEN_PTR) return strdup("ptr");

    if (kind >= TOKEN_EAX && kind <= TOKEN_RDI) return strdup("register");

    if (kind == TOKEN_X86_64_NOP) return strdup("nop");
    if (kind == TOKEN_X86_64_MOV) return strdup("mov");
    if (kind == TOKEN_X86_64_LEA) return strdup("lea");
    if (kind == TOKEN_X86_64_ADD) return strdup("add");
    if (kind == TOKEN_X86_64_ADC) return strdup("add");
    if (kind == TOKEN_X86_64_SUB) return strdup("sub");
    if (kind == TOKEN_X86_64_SBB) return strdup("sbb");
    if (kind == TOKEN_X86_64_CMP) return strdup("cmp");
    if (kind == TOKEN_X86_64_AND) return strdup("and");
    if (kind == TOKEN_X86_64_OR) return strdup("or");
    if (kind == TOKEN_X86_64_XOR) return strdup("xor");
    if (kind == TOKEN_X86_64_NEG) return strdup("neg");
    if (kind == TOKEN_X86_64_PUSH) return strdup("push");
    if (kind == TOKEN_X86_64_POP) return strdup("pop");
    if (kind == TOKEN_X86_64_SYSCALL) return strdup("syscall");
    if (kind == TOKEN_X86_64_CDQ) return strdup("cdq");
    if (kind == TOKEN_X86_64_CQO) return strdup("cqo");
    if (kind == TOKEN_X86_64_LEAVE) return strdup("leave");
    if (kind == TOKEN_X86_64_RET) return strdup("ret");
    return NULL;
}
