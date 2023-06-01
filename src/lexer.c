#include "lexer.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

// Token
char *token_type_to_string(TokenType type) {
    if (type == TOKEN_EOF) return "EOF";
    if (type == TOKEN_UNKNOWN) return "unknown?";

    if (type == TOKEN_VARIABLE) return "variable";
    if (type == TOKEN_INTEGER) return "integer";

    if (type == TOKEN_RETURN) return "return";

    if (type == TOKEN_LPAREN) return "(";
    if (type == TOKEN_RPAREN) return ")";
    if (type == TOKEN_LCURLY) return "{";
    if (type == TOKEN_RCURLY) return "}";
    if (type == TOKEN_COMMA) return ",";
    if (type == TOKEN_SEMICOLON) return ";";

    if (type == TOKEN_NOT) return "~";
    if (type == TOKEN_LOGICAL_NOT) return "~";

    if (type == TOKEN_ASSIGN) return "=";
    if (type == TOKEN_ADD) return "+";
    if (type == TOKEN_SUB) return "-";
    if (type == TOKEN_MUL) return "*";
    if (type == TOKEN_DIV) return "/";
    if (type == TOKEN_MOD) return "%";
    if (type == TOKEN_AND) return "&";
    if (type == TOKEN_OR) return "|";
    if (type == TOKEN_XOR) return "^";
    if (type == TOKEN_SHL) return "<<";
    if (type == TOKEN_SHR) return ">>";
    if (type == TOKEN_EQ) return "=";
    if (type == TOKEN_NEQ) return "!=";
    if (type == TOKEN_LT) return "<";
    if (type == TOKEN_LTEQ) return "<=";
    if (type == TOKEN_GT) return ">";
    if (type == TOKEN_GTEQ) return ">=";
    if (type == TOKEN_LOGICAL_AND) return "&&";
    if (type == TOKEN_LOGICAL_OR) return "||";

    return NULL;
}

// Lexer
Keyword keywords[] = {{"return", TOKEN_RETURN}};

Token *lexer(char *text, size_t *tokens_size) {
    size_t capacity = 1024;
    Token *tokens = malloc(capacity * sizeof(Token));
    size_t size = 0;

    char *c = text;
    char *line_start = c;
    int32_t line = 1;
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

        // Comments
        if (*c == '/' && *(c + 1) == '/') {
            while (*c != '\n' && *c != '\r') c++;
            continue;
        }
        if (*c == '/' && *(c + 1) == '*') {
            c += 2;
            while (*c != '*' || *(c + 1) != '/') {
                if (*c == '\n' || *c == '\r') {
                    if (*c == '\r') c++;
                    line_start = ++c;
                    line++;
                    continue;
                }
                c++;
            }
            c += 2;
            continue;
        }

        // Whitespace
        if (*c == ' ' || *c == '\t') {
            while (*c == ' ' || *c == '\t') c++;
            continue;
        }
        if (*c == '\r' || *c == '\n') {
            if (*c == '\r') c++;
            line_start = ++c;
            line++;
            continue;
        }

        // Integers
        if (*c == '0' && *(c + 1) == 'b') {
            c += 2;
            tokens[size].type = TOKEN_INTEGER;
            tokens[size++].integer = strtol(c, &c, 2);
            continue;
        }
        if (*c == '0' && (isdigit(*(c + 1)) || *(c + 1) == 'o')) {
            if (*(c + 1) == 'o') c++;
            c++;
            tokens[size].type = TOKEN_INTEGER;
            tokens[size++].integer = strtol(c, &c, 8);
            continue;
        }
        if (*c == '0' && *(c + 1) == 'x') {
            c += 2;
            tokens[size].type = TOKEN_INTEGER;
            tokens[size++].integer = strtol(c, &c, 16);
            continue;
        }
        if (isdigit(*c)) {
            tokens[size].type = TOKEN_INTEGER;
            tokens[size++].integer = strtol(c, &c, 10);
            continue;
        }

        // Variables
        if (isalpha(*c) || *c == '_') {
            char *string = c;
            while (isalnum(*c) || *c == '_') c++;
            size_t string_size = c - string;

            bool found = false;
            for (size_t i = 0; i < sizeof(keywords) / sizeof(Keyword); i++) {
                Keyword *keyword = &keywords[i];
                size_t keywordSize = strlen(keyword->keyword);
                if (!memcmp(string, keyword->keyword, keywordSize) && string_size == keywordSize) {
                    tokens[size++].type = keyword->type;
                    found = true;
                    break;
                }
            }
            if (!found) {
                tokens[size].type = TOKEN_VARIABLE;
                tokens[size++].variable = strndup(string, string_size);
            }
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
        if (*c == '{') {
            tokens[size++].type = TOKEN_LCURLY;
            c++;
            continue;
        }
        if (*c == '}') {
            tokens[size++].type = TOKEN_RCURLY;
            c++;
            continue;
        }
        if (*c == ',') {
            tokens[size++].type = TOKEN_COMMA;
            c++;
            continue;
        }
        if (*c == ';') {
            tokens[size++].type = TOKEN_SEMICOLON;
            c++;
            continue;
        }

        // Operators
        if (*c == '~') {
            tokens[size++].type = TOKEN_NOT;
            c++;
            continue;
        }
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

            tokens[size++].type = TOKEN_ASSIGN;
            c++;
            continue;
        }
        if (*c == '!') {
            if (*(c + 1) == '=') {
                tokens[size++].type = TOKEN_NEQ;
                c += 2;
                continue;
            }

            tokens[size++].type = TOKEN_LOGICAL_NOT;
            c++;
            continue;
        }
        if (*c == '<') {
            if (*(c + 1) == '<') {
                tokens[size++].type = TOKEN_SHL;
                c += 2;
                continue;
            }
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
            if (*(c + 1) == '>') {
                tokens[size++].type = TOKEN_SHR;
                c += 2;
                continue;
            }
            if (*(c + 1) == '=') {
                tokens[size++].type = TOKEN_GTEQ;
                c += 2;
                continue;
            }

            tokens[size++].type = TOKEN_GT;
            c++;
            continue;
        }
        if (*c == '&') {
            if (*(c + 1) == '&') {
                tokens[size++].type = TOKEN_LOGICAL_AND;
                c += 2;
                continue;
            }

            tokens[size++].type = TOKEN_AND;
            c++;
            continue;
        }
        if (*c == '|') {
            if (*(c + 1) == '|') {
                tokens[size++].type = TOKEN_LOGICAL_OR;
                c += 2;
                continue;
            }

            tokens[size++].type = TOKEN_OR;
            c++;
            continue;
        }
        if (*c == '^') {
            tokens[size++].type = TOKEN_XOR;
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
