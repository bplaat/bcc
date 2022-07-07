#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

char *text;
char *c;

Token *token_new(TokenType type) {
    Token *token = malloc(sizeof(Token));
    token->type = type;
    token->position = c - text;
    return token;
}

void token_type_to_string(TokenType type, char *buffer) {
    if (type == TOKEN_TYPE_EOF) strcpy(buffer, "EOF");
    if (type == TOKEN_TYPE_NUMBER) strcpy(buffer, "number");
    if (type == TOKEN_TYPE_VARIABLE) strcpy(buffer, "variable");
    if (type == TOKEN_TYPE_LPAREN) strcpy(buffer, "(");
    if (type == TOKEN_TYPE_RPAREN) strcpy(buffer, ")");
    if (type == TOKEN_TYPE_LCURLY) strcpy(buffer, "{");
    if (type == TOKEN_TYPE_RCURLY) strcpy(buffer, "}");
    if (type == TOKEN_TYPE_SEMICOLON) strcpy(buffer, ";");

    if (type == TOKEN_TYPE_IF) strcpy(buffer, "if");
    if (type == TOKEN_TYPE_ELSE) strcpy(buffer, "else");
    if (type == TOKEN_TYPE_WHILE) strcpy(buffer, "while");
    if (type == TOKEN_TYPE_FOR) strcpy(buffer, "for");
    if (type == TOKEN_TYPE_RETURN) strcpy(buffer, "return");

    if (type == TOKEN_TYPE_ASSIGN) strcpy(buffer, "=");
    if (type == TOKEN_TYPE_ADD) strcpy(buffer, "+");
    if (type == TOKEN_TYPE_SUB) strcpy(buffer, "-");
    if (type == TOKEN_TYPE_STAR) strcpy(buffer, "*");
    if (type == TOKEN_TYPE_DIV) strcpy(buffer, "/");
    if (type == TOKEN_TYPE_MOD) strcpy(buffer, "%");
    if (type == TOKEN_TYPE_ADDR) strcpy(buffer, "&");
    if (type == TOKEN_TYPE_EQ) strcpy(buffer, "==");
    if (type == TOKEN_TYPE_NEQ) strcpy(buffer, "!=");
    if (type == TOKEN_TYPE_LT) strcpy(buffer, "<");
    if (type == TOKEN_TYPE_LTEQ) strcpy(buffer, "<=");
    if (type == TOKEN_TYPE_GT) strcpy(buffer, ">");
    if (type == TOKEN_TYPE_GTEQ) strcpy(buffer, ">=");
    if (type == TOKEN_TYPE_LOGIC_NOT) strcpy(buffer, "!");
    if (type == TOKEN_TYPE_LOGIC_AND) strcpy(buffer, "&&");
    if (type == TOKEN_TYPE_LOGIC_OR) strcpy(buffer, "||");
}

List *lexer(char *_text) {
    text = _text;
    c = text;
    List *tokens = list_new(512);

    while (*c != '\0') {
        if (*c == ' ') {
            c++;
            continue;
        }

        if (isdigit(*c)) {
            Token *token = token_new(TOKEN_TYPE_NUMBER);
            token->number = strtol(c, &c, 10);
            list_add(tokens, token);
            continue;
        }

        if (!strncmp(c, "if", 2)) {
            list_add(tokens, token_new(TOKEN_TYPE_IF));
            c += 2;
            continue;
        }
        if (!strncmp(c, "else", 4)) {
            list_add(tokens, token_new(TOKEN_TYPE_ELSE));
            c += 4;
            continue;
        }
        if (!strncmp(c, "while", 5)) {
            list_add(tokens, token_new(TOKEN_TYPE_WHILE));
            c += 5;
            continue;
        }
        if (!strncmp(c, "for", 3)) {
            list_add(tokens, token_new(TOKEN_TYPE_FOR));
            c += 3;
            continue;
        }
        if (!strncmp(c, "return", 6)) {
            list_add(tokens, token_new(TOKEN_TYPE_RETURN));
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
            Token *token = token_new(TOKEN_TYPE_VARIABLE);
            token->string = malloc(size + 1);
            memcpy(token->string, ptr, size);
            token->string[size] = '\0';
            list_add(tokens, token);
            continue;
        }

        if (*c == '(') {
            list_add(tokens, token_new(TOKEN_TYPE_LPAREN));
            c++;
            continue;
        }
        if (*c == ')') {
            list_add(tokens, token_new(TOKEN_TYPE_RPAREN));
            c++;
            continue;
        }
        if (*c == '{') {
            list_add(tokens, token_new(TOKEN_TYPE_LCURLY));
            c++;
            continue;
        }
        if (*c == '}') {
            list_add(tokens, token_new(TOKEN_TYPE_RCURLY));
            c++;
            continue;
        }
        if (*c == ';') {
            list_add(tokens, token_new(TOKEN_TYPE_SEMICOLON));
            c++;
            continue;
        }

        if (*c == '+') {
            list_add(tokens, token_new(TOKEN_TYPE_ADD));
            c++;
            continue;
        }
        if (*c == '-') {
            list_add(tokens, token_new(TOKEN_TYPE_SUB));
            c++;
            continue;
        }
        if (*c == '*') {
            list_add(tokens, token_new(TOKEN_TYPE_STAR));
            c++;
            continue;
        }
        if (*c == '/') {
            list_add(tokens, token_new(TOKEN_TYPE_DIV));
            c++;
            continue;
        }
        if (*c == '%') {
            list_add(tokens, token_new(TOKEN_TYPE_MOD));
            c++;
            continue;
        }
        if (*c == '&') {
            list_add(tokens, token_new(TOKEN_TYPE_ADDR));
            c++;
            continue;
        }

        if (*c == '=') {
            if (*(c + 1) == '=') {
                list_add(tokens, token_new(TOKEN_TYPE_EQ));
                c += 2;
                continue;
            }

            list_add(tokens, token_new(TOKEN_TYPE_ASSIGN));
            c++;
            continue;
        }
        if (*c == '!') {
            if (*(c + 1) == '=') {
                list_add(tokens, token_new(TOKEN_TYPE_NEQ));
                c += 2;
                continue;
            }

            list_add(tokens, token_new(TOKEN_TYPE_LOGIC_NOT));
            c++;
            continue;
        }
        if (*c == '<') {
            if (*(c + 1) == '=') {
                list_add(tokens, token_new(TOKEN_TYPE_LTEQ));
                c += 2;
                continue;
            }

            list_add(tokens, token_new(TOKEN_TYPE_LT));
            c++;
            continue;
        }
        if (*c == '>') {
            if (*(c + 1) == '=') {
                list_add(tokens, token_new(TOKEN_TYPE_GTEQ));
                c += 2;
                continue;
            }

            list_add(tokens, token_new(TOKEN_TYPE_GT));
            c++;
            continue;
        }
        if (*c == '&' && *(c + 1) == '&') {
            list_add(tokens, token_new(TOKEN_TYPE_LOGIC_AND));
            c += 2;
            continue;
        }
        if (*c == '|' && *(c + 1) == '|') {
            list_add(tokens, token_new(TOKEN_TYPE_LOGIC_OR));
            c += 2;
            continue;
        }

        fprintf(stderr, "%s\n", text);
        for (int32_t i = 0; i < c - text; i++) fprintf(stderr, " ");
        fprintf(stderr, "^\nUnexpected character: '%c'\n", *c);
        exit(EXIT_FAILURE);
    }

    list_add(tokens, token_new(TOKEN_TYPE_EOF));
    return tokens;
}
