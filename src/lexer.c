#include "lexer.h"

#include <ctype.h>
#include <stdlib.h>

Token *lexer(char *text, size_t *tokens_size) {
    size_t capacity = 1024;
    Token *tokens = malloc(capacity * sizeof(Token));
    size_t size = 0;
    char *c = text;
    int32_t line = 1;
    char *line_start = c;
    for (;;) {
        tokens[size].line = line;
        tokens[size].column = c - line_start + 1;

        if (size == capacity) {
            capacity *= 2;
            tokens = realloc(tokens, capacity * sizeof(Token));
        }

        // EOF
        if (*c == '\0') {
            tokens[size++].type = TOKEN_EOF;
            break;
        }

        // Whitespace
        if (*c == ' ' || *c == '\t') {
            while (*c == ' ' || *c == '\t') c++;
            continue;
        }
        if (*c == '\r' || *c == '\n') {
            if (*c == '\r') c++;
            c++;
            line_start = c;
            continue;
        }

        // Literals
        if (isdigit(*c)) {
            tokens[size].type = TOKEN_INTEGER;
            tokens[size++].integer = strtol(c, &c, 10);
            continue;
        }

        // Syntax
        if (*c == '(') {
            tokens[size++].type = TOKEN_LPAREN;
            c++;
            continue;
        }
        if (*c == ')') {
            tokens[size++].type = TOKEN_RPAREN;
            c++;
            continue;
        }

        // Operators
        if (*c == '+') {
            tokens[size++].type = TOKEN_ADD;
            c++;
            continue;
        }
        if (*c == '-') {
            tokens[size++].type = TOKEN_SUB;
            c++;
            continue;
        }
        if (*c == '*') {
            tokens[size++].type = TOKEN_MUL;
            c++;
            continue;
        }
        if (*c == '/') {
            tokens[size++].type = TOKEN_DIV;
            c++;
            continue;
        }
        if (*c == '%') {
            tokens[size++].type = TOKEN_MOD;
            c++;
            continue;
        }
        if (*c == '=') {
            if (*(c + 1) == '=') {
                tokens[size++].type = TOKEN_EQ;
                c += 2;
                continue;
            }
        }
        if (*c == '!') {
            if (*(c + 1) == '=') {
                tokens[size++].type = TOKEN_NEQ;
                c += 2;
                continue;
            }
        }
        if (*c == '<') {
            if (*(c + 1) == '=') {
                tokens[size++].type = TOKEN_LTEQ;
                c += 2;
                continue;
            }

            tokens[size++].type = TOKEN_LT;
            c++;
            continue;
        }
        if (*c == '>') {
            if (*(c + 1) == '=') {
                tokens[size++].type = TOKEN_GTEQ;
                c += 2;
                continue;
            }

            tokens[size++].type = TOKEN_GT;
            c++;
            continue;
        }

        // Unknown character
        tokens[size].type = TOKEN_UNKNOWN;
        tokens[size].character = *c++;
    }
    *tokens_size = size;
    return tokens;
}
