#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

char *text;
char *tokenStart;

Token *token_new(TokenKind kind) {
    Token *token = malloc(sizeof(Token));
    token->kind = kind;
    token->position = tokenStart - text;
    return token;
}

char *token_to_string(TokenKind kind) {
    if (kind == TOKEN_EOF) return strdup("EOF");
    if (kind == TOKEN_INTEGER) return strdup("integer");
    if (kind == TOKEN_VARIABLE) return strdup("variable");
    if (kind == TOKEN_LPAREN) return strdup("(");
    if (kind == TOKEN_RPAREN) return strdup(")");
    if (kind == TOKEN_LCURLY) return strdup("{");
    if (kind == TOKEN_RCURLY) return strdup("}");
    if (kind == TOKEN_LBRACKET) return strdup("[");
    if (kind == TOKEN_RBRACKET) return strdup("]");
    if (kind == TOKEN_COMMA) return strdup(",");
    if (kind == TOKEN_SEMICOLON) return strdup(";");

    if (kind == TOKEN_INT) return strdup("int");
    if (kind == TOKEN_LONG) return strdup("long");
    if (kind == TOKEN_IF) return strdup("if");
    if (kind == TOKEN_ELSE) return strdup("else");
    if (kind == TOKEN_WHILE) return strdup("while");
    if (kind == TOKEN_FOR) return strdup("for");
    if (kind == TOKEN_RETURN) return strdup("return");

    if (kind == TOKEN_ASSIGN) return strdup("=");
    if (kind == TOKEN_ADD) return strdup("+");
    if (kind == TOKEN_SUB) return strdup("-");
    if (kind == TOKEN_STAR) return strdup("*");
    if (kind == TOKEN_DIV) return strdup("/");
    if (kind == TOKEN_MOD) return strdup("%");
    if (kind == TOKEN_ADDR) return strdup("&");
    if (kind == TOKEN_EQ) return strdup("==");
    if (kind == TOKEN_NEQ) return strdup("!=");
    if (kind == TOKEN_LT) return strdup("<");
    if (kind == TOKEN_LTEQ) return strdup("<=");
    if (kind == TOKEN_GT) return strdup(">");
    if (kind == TOKEN_GTEQ) return strdup(">=");
    if (kind == TOKEN_LOGIC_NOT) return strdup("!");
    if (kind == TOKEN_LOGIC_AND) return strdup("&&");
    if (kind == TOKEN_LOGIC_OR) return strdup("||");
    return NULL;
}

List *lexer(char *_text) {
    text = _text;
    char *c = text;
    List *tokens = list_new(512);

    while (*c != '\0') {
        tokenStart = c;

        if (*c == ' ') {
            c++;
            continue;
        }

        if (isdigit(*c)) {
            Token *token = token_new(TOKEN_INTEGER);
            token->integer = strtol(c, &c, 10);
            list_add(tokens, token);
            continue;
        }

        if (!strncmp(c, "int", 3)) {
            list_add(tokens, token_new(TOKEN_INT));
            c += 3;
            continue;
        }
        if (!strncmp(c, "long", 4)) {
            list_add(tokens, token_new(TOKEN_LONG));
            c += 4;
            continue;
        }
        if (!strncmp(c, "signed", 6)) {
            list_add(tokens, token_new(TOKEN_SIGNED));
            c += 6;
            continue;
        }
        if (!strncmp(c, "unsigned", 8)) {
            list_add(tokens, token_new(TOKEN_UNSIGNED));
            c += 8;
            continue;
        }
        if (!strncmp(c, "if", 2)) {
            list_add(tokens, token_new(TOKEN_IF));
            c += 2;
            continue;
        }
        if (!strncmp(c, "else", 4)) {
            list_add(tokens, token_new(TOKEN_ELSE));
            c += 4;
            continue;
        }
        if (!strncmp(c, "while", 5)) {
            list_add(tokens, token_new(TOKEN_WHILE));
            c += 5;
            continue;
        }
        if (!strncmp(c, "for", 3)) {
            list_add(tokens, token_new(TOKEN_FOR));
            c += 3;
            continue;
        }
        if (!strncmp(c, "return", 6)) {
            list_add(tokens, token_new(TOKEN_RETURN));
            c += 6;
            continue;
        }

        if (isalpha(*c) || *c == '_') {
            char *ptr = c;
            size_t size = 0;
            while (isalnum(*c) || *c == '_') {
                size++;
                c++;
            }
            Token *token = token_new(TOKEN_VARIABLE);
            token->string = malloc(size + 1);
            memcpy(token->string, ptr, size);
            token->string[size] = '\0';
            list_add(tokens, token);
            continue;
        }

        if (*c == '(') {
            list_add(tokens, token_new(TOKEN_LPAREN));
            c++;
            continue;
        }
        if (*c == ')') {
            list_add(tokens, token_new(TOKEN_RPAREN));
            c++;
            continue;
        }
        if (*c == '{') {
            list_add(tokens, token_new(TOKEN_LCURLY));
            c++;
            continue;
        }
        if (*c == '}') {
            list_add(tokens, token_new(TOKEN_RCURLY));
            c++;
            continue;
        }
        if (*c == '[') {
            list_add(tokens, token_new(TOKEN_LBRACKET));
            c++;
            continue;
        }
        if (*c == ']') {
            list_add(tokens, token_new(TOKEN_RBRACKET));
            c++;
            continue;
        }
        if (*c == ',') {
            list_add(tokens, token_new(TOKEN_COMMA));
            c++;
            continue;
        }
        if (*c == ';') {
            list_add(tokens, token_new(TOKEN_SEMICOLON));
            c++;
            continue;
        }

        if (*c == '+') {
            list_add(tokens, token_new(TOKEN_ADD));
            c++;
            continue;
        }
        if (*c == '-') {
            list_add(tokens, token_new(TOKEN_SUB));
            c++;
            continue;
        }
        if (*c == '*') {
            list_add(tokens, token_new(TOKEN_STAR));
            c++;
            continue;
        }
        if (*c == '/') {
            list_add(tokens, token_new(TOKEN_DIV));
            c++;
            continue;
        }
        if (*c == '%') {
            list_add(tokens, token_new(TOKEN_MOD));
            c++;
            continue;
        }
        if (*c == '&') {
            if (*(c + 1) == '&') {
                list_add(tokens, token_new(TOKEN_LOGIC_AND));
                c += 2;
                continue;
            }

            list_add(tokens, token_new(TOKEN_ADDR));
            c++;
            continue;
        }

        if (*c == '=') {
            if (*(c + 1) == '=') {
                list_add(tokens, token_new(TOKEN_EQ));
                c += 2;
                continue;
            }

            list_add(tokens, token_new(TOKEN_ASSIGN));
            c++;
            continue;
        }
        if (*c == '!') {
            if (*(c + 1) == '=') {
                list_add(tokens, token_new(TOKEN_NEQ));
                c += 2;
                continue;
            }

            list_add(tokens, token_new(TOKEN_LOGIC_NOT));
            c++;
            continue;
        }
        if (*c == '<') {
            if (*(c + 1) == '=') {
                list_add(tokens, token_new(TOKEN_LTEQ));
                c += 2;
                continue;
            }

            list_add(tokens, token_new(TOKEN_LT));
            c++;
            continue;
        }
        if (*c == '>') {
            if (*(c + 1) == '=') {
                list_add(tokens, token_new(TOKEN_GTEQ));
                c += 2;
                continue;
            }

            list_add(tokens, token_new(TOKEN_GT));
            c++;
            continue;
        }
        if (*c == '|' && *(c + 1) == '|') {
            list_add(tokens, token_new(TOKEN_LOGIC_OR));
            c += 2;
            continue;
        }

        fprintf(stderr, "%s\n", text);
        for (int32_t i = 0; i < tokenStart - text; i++) fprintf(stderr, " ");
        fprintf(stderr, "^\nUnexpected character: '%c'\n", *c);
        exit(EXIT_FAILURE);
    }

    list_add(tokens, token_new(TOKEN_EOF));
    return tokens;
}
