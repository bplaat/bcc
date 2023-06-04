#include "lexer.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

// Token
char *token_kind_to_string(TokenKind kind) {
    if (kind == TOKEN_EOF) return "EOF";
    if (kind == TOKEN_UNKNOWN) return "unknown?";

    if (kind == TOKEN_VARIABLE) return "variable";
    if (kind == TOKEN_INTEGER) return "integer";

    if (kind == TOKEN_INT) return "int";
    if (kind == TOKEN_SIZEOF) return "sizeof";
    if (kind == TOKEN_IF) return "if";
    if (kind == TOKEN_ELSE) return "else";
    if (kind == TOKEN_WHILE) return "while";
    if (kind == TOKEN_DO) return "do";
    if (kind == TOKEN_FOR) return "for";
    if (kind == TOKEN_RETURN) return "return";

    if (kind == TOKEN_LPAREN) return "(";
    if (kind == TOKEN_RPAREN) return ")";
    if (kind == TOKEN_LBLOCK) return "[";
    if (kind == TOKEN_RBLOCK) return "]";
    if (kind == TOKEN_LCURLY) return "{";
    if (kind == TOKEN_RCURLY) return "}";
    if (kind == TOKEN_COMMA) return ",";
    if (kind == TOKEN_QUESTION) return "?";
    if (kind == TOKEN_COLON) return ":";
    if (kind == TOKEN_SEMICOLON) return ";";

    if (kind == TOKEN_NOT) return "~";
    if (kind == TOKEN_LOGICAL_NOT) return "~";

    if (kind == TOKEN_ASSIGN) return "=";
    if (kind == TOKEN_ADD_ASSIGN) return "+=";
    if (kind == TOKEN_SUB_ASSIGN) return "-=";
    if (kind == TOKEN_MUL_ASSIGN) return "*=";
    if (kind == TOKEN_DIV_ASSIGN) return "/=";
    if (kind == TOKEN_MOD_ASSIGN) return "%=";
    if (kind == TOKEN_AND_ASSIGN) return "&=";
    if (kind == TOKEN_OR_ASSIGN) return "|=";
    if (kind == TOKEN_XOR_ASSIGN) return "^=";
    if (kind == TOKEN_SHL_ASSIGN) return "<<=";
    if (kind == TOKEN_SHR_ASSIGN) return ">>=";
    if (kind == TOKEN_ADD) return "+";
    if (kind == TOKEN_SUB) return "-";
    if (kind == TOKEN_MUL) return "*";
    if (kind == TOKEN_DIV) return "/";
    if (kind == TOKEN_MOD) return "%";
    if (kind == TOKEN_AND) return "&";
    if (kind == TOKEN_OR) return "|";
    if (kind == TOKEN_XOR) return "^";
    if (kind == TOKEN_SHL) return "<<";
    if (kind == TOKEN_SHR) return ">>";
    if (kind == TOKEN_EQ) return "=";
    if (kind == TOKEN_NEQ) return "!=";
    if (kind == TOKEN_LT) return "<";
    if (kind == TOKEN_LTEQ) return "<=";
    if (kind == TOKEN_GT) return ">";
    if (kind == TOKEN_GTEQ) return ">=";
    if (kind == TOKEN_LOGICAL_AND) return "&&";
    if (kind == TOKEN_LOGICAL_OR) return "||";

    return NULL;
}

// Lexer
Keyword keywords[] = {{"int", TOKEN_INT},     {"sizeof", TOKEN_SIZEOF}, {"if", TOKEN_IF},   {"else", TOKEN_ELSE},
                      {"while", TOKEN_WHILE}, {"do", TOKEN_DO},       {"for", TOKEN_FOR}, {"return", TOKEN_RETURN}};

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
            tokens[size++].kind = TOKEN_EOF;
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
            tokens[size].kind = TOKEN_INTEGER;
            tokens[size++].integer = strtol(c, &c, 2);
            continue;
        }
        if (*c == '0' && (isdigit(*(c + 1)) || *(c + 1) == 'o')) {
            if (*(c + 1) == 'o') c++;
            c++;
            tokens[size].kind = TOKEN_INTEGER;
            tokens[size++].integer = strtol(c, &c, 8);
            continue;
        }
        if (*c == '0' && *(c + 1) == 'x') {
            c += 2;
            tokens[size].kind = TOKEN_INTEGER;
            tokens[size++].integer = strtol(c, &c, 16);
            continue;
        }
        if (isdigit(*c)) {
            tokens[size].kind = TOKEN_INTEGER;
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
                    tokens[size++].kind = keyword->kind;
                    found = true;
                    break;
                }
            }
            if (!found) {
                tokens[size].kind = TOKEN_VARIABLE;
                tokens[size++].variable = strndup(string, string_size);
            }
            continue;
        }

        // Syntax
        if (*c == '(') {
            tokens[size++].kind = TOKEN_LPAREN;
            c++;
            continue;
        }
        if (*c == ')') {
            tokens[size++].kind = TOKEN_RPAREN;
            c++;
            continue;
        }
        if (*c == '[') {
            tokens[size++].kind = TOKEN_LBLOCK;
            c++;
            continue;
        }
        if (*c == ']') {
            tokens[size++].kind = TOKEN_RBLOCK;
            c++;
            continue;
        }
        if (*c == '{') {
            tokens[size++].kind = TOKEN_LCURLY;
            c++;
            continue;
        }
        if (*c == '}') {
            tokens[size++].kind = TOKEN_RCURLY;
            c++;
            continue;
        }
        if (*c == ',') {
            tokens[size++].kind = TOKEN_COMMA;
            c++;
            continue;
        }
        if (*c == '?') {
            tokens[size++].kind = TOKEN_QUESTION;
            c++;
            continue;
        }
        if (*c == ':') {
            tokens[size++].kind = TOKEN_COLON;
            c++;
            continue;
        }
        if (*c == ';') {
            tokens[size++].kind = TOKEN_SEMICOLON;
            c++;
            continue;
        }

        // Operators
        if (*c == '~') {
            tokens[size++].kind = TOKEN_NOT;
            c++;
            continue;
        }
        if (*c == '+') {
            if (*(c + 1) == '=') {
                tokens[size++].kind = TOKEN_ADD_ASSIGN;
                c += 2;
                continue;
            }

            tokens[size++].kind = TOKEN_ADD;
            c++;
            continue;
        }
        if (*c == '-') {
            if (*(c + 1) == '=') {
                tokens[size++].kind = TOKEN_SUB_ASSIGN;
                c += 2;
                continue;
            }

            tokens[size++].kind = TOKEN_SUB;
            c++;
            continue;
        }
        if (*c == '*') {
            if (*(c + 1) == '=') {
                tokens[size++].kind = TOKEN_MUL_ASSIGN;
                c += 2;
                continue;
            }

            tokens[size++].kind = TOKEN_MUL;
            c++;
            continue;
        }
        if (*c == '/') {
            if (*(c + 1) == '=') {
                tokens[size++].kind = TOKEN_DIV_ASSIGN;
                c += 2;
                continue;
            }

            tokens[size++].kind = TOKEN_DIV;
            c++;
            continue;
        }
        if (*c == '%') {
            if (*(c + 1) == '=') {
                tokens[size++].kind = TOKEN_MOD_ASSIGN;
                c += 2;
                continue;
            }

            tokens[size++].kind = TOKEN_MOD;
            c++;
            continue;
        }
        if (*c == '=') {
            if (*(c + 1) == '=') {
                tokens[size++].kind = TOKEN_EQ;
                c += 2;
                continue;
            }

            tokens[size++].kind = TOKEN_ASSIGN;
            c++;
            continue;
        }
        if (*c == '!') {
            if (*(c + 1) == '=') {
                tokens[size++].kind = TOKEN_NEQ;
                c += 2;
                continue;
            }

            tokens[size++].kind = TOKEN_LOGICAL_NOT;
            c++;
            continue;
        }
        if (*c == '<') {
            if (*(c + 1) == '<') {
                if (*(c + 2) == '=') {
                    tokens[size++].kind = TOKEN_SHL_ASSIGN;
                    c += 3;
                    continue;
                }

                tokens[size++].kind = TOKEN_SHL;
                c += 2;
                continue;
            }
            if (*(c + 1) == '=') {
                tokens[size++].kind = TOKEN_LTEQ;
                c += 2;
                continue;
            }

            tokens[size++].kind = TOKEN_LT;
            c++;
            continue;
        }
        if (*c == '>') {
            if (*(c + 1) == '>') {
                if (*(c + 2) == '=') {
                    tokens[size++].kind = TOKEN_SHR_ASSIGN;
                    c += 3;
                    continue;
                }

                tokens[size++].kind = TOKEN_SHR;
                c += 2;
                continue;
            }
            if (*(c + 1) == '=') {
                tokens[size++].kind = TOKEN_GTEQ;
                c += 2;
                continue;
            }

            tokens[size++].kind = TOKEN_GT;
            c++;
            continue;
        }
        if (*c == '&') {
            if (*(c + 1) == '=') {
                tokens[size++].kind = TOKEN_AND_ASSIGN;
                c += 2;
                continue;
            }

            if (*(c + 1) == '&') {
                tokens[size++].kind = TOKEN_LOGICAL_AND;
                c += 2;
                continue;
            }

            tokens[size++].kind = TOKEN_AND;
            c++;
            continue;
        }
        if (*c == '|') {
            if (*(c + 1) == '=') {
                tokens[size++].kind = TOKEN_OR_ASSIGN;
                c += 2;
                continue;
            }

            if (*(c + 1) == '|') {
                tokens[size++].kind = TOKEN_LOGICAL_OR;
                c += 2;
                continue;
            }

            tokens[size++].kind = TOKEN_OR;
            c++;
            continue;
        }
        if (*c == '^') {
            if (*(c + 1) == '=') {
                tokens[size++].kind = TOKEN_XOR_ASSIGN;
                c += 2;
                continue;
            }

            tokens[size++].kind = TOKEN_XOR;
            c++;
            continue;
        }

        // Unknown character
        tokens[size].kind = TOKEN_UNKNOWN;
        tokens[size++].character = *c++;
    }
    *tokens_size = size;
    return tokens;
}
