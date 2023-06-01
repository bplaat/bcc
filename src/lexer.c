#include "lexer.h"

#include <ctype.h>
#include <stdlib.h>

Token *lexer(char *text, size_t *tokens_size) {
    size_t capacity = 1024;
    Token *tokens = malloc(capacity * sizeof(Token));
    size_t size = 0;
    char *c = text;
    for (;;) {
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
        if (isspace(*c)) {
            while (isspace(*c)) c++;
            continue;
        }

        // Literals
        if (isdigit(*c)) {
            tokens[size].type = TOKEN_INTEGER;
            tokens[size++].integer = strtol(c, &c, 10);
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

        // Unknown character
        tokens[size].type = TOKEN_UNKNOWN;
        tokens[size].character = *c++;
    }
    *tokens_size = size;
    return tokens;
}
