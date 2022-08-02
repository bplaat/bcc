#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

Token *token_new(TokenKind kind, size_t position) {
    Token *token = malloc(sizeof(Token));
    token->kind = kind;
    token->position = position;
    return token;
}

Token *token_new_integer(TokenKind kind, size_t position, int64_t integer) {
    Token *token = token_new(kind, position);
    token->integer = integer;
    return token;
}

Token *token_new_string(TokenKind kind, size_t position, char *string) {
    Token *token = token_new(kind, position);
    token->string = string;
    return token;
}

char *token_kind_to_string(TokenKind kind) {
    if (kind == TOKEN_INTEGER) return strdup("integer");
    if (kind == TOKEN_CHARACTER) return strdup("character");
    if (kind == TOKEN_STRING) return strdup("string");
    if (kind == TOKEN_VARIABLE) return strdup("variable");

    if (kind == TOKEN_LPAREN) return strdup("(");
    if (kind == TOKEN_RPAREN) return strdup(")");
    if (kind == TOKEN_LCURLY) return strdup("{");
    if (kind == TOKEN_RCURLY) return strdup("}");
    if (kind == TOKEN_LBRACKET) return strdup("[");
    if (kind == TOKEN_RBRACKET) return strdup("]");
    if (kind == TOKEN_COMMA) return strdup(",");
    if (kind == TOKEN_SEMICOLON) return strdup(";");
    if (kind == TOKEN_EOF) return strdup("EOF");

    if (kind == TOKEN_CHAR) return strdup("char");
    if (kind == TOKEN_SHORT) return strdup("short");
    if (kind == TOKEN_INT) return strdup("int");
    if (kind == TOKEN_LONG) return strdup("long");
    if (kind == TOKEN_SIGNED) return strdup("signed");
    if (kind == TOKEN_UNSIGNED) return strdup("unsigned");
    if (kind == TOKEN_SIZEOF) return strdup("sizeof");
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
    if (kind == TOKEN_AND) return strdup("&");
    if (kind == TOKEN_EQ) return strdup("==");
    if (kind == TOKEN_NEQ) return strdup("!=");
    if (kind == TOKEN_LT) return strdup("<");
    if (kind == TOKEN_LTEQ) return strdup("<=");
    if (kind == TOKEN_GT) return strdup(">");
    if (kind == TOKEN_GTEQ) return strdup(">=");
    if (kind == TOKEN_LOGIC_OR) return strdup("||");
    if (kind == TOKEN_LOGIC_AND) return strdup("&&");
    if (kind == TOKEN_LOGIC_NOT) return strdup("!");
    return NULL;
}

bool token_kind_is_type(TokenKind kind) {
    return kind == TOKEN_CHAR || kind == TOKEN_SHORT || kind == TOKEN_INT || kind == TOKEN_LONG || kind == TOKEN_SIGNED || kind == TOKEN_UNSIGNED;
}

List *lexer(char *text) {
    List *tokens = list_new(1024);
    char *c = text;
    Keyword keywords[] = {{"char", TOKEN_CHAR},     {"short", TOKEN_SHORT},       {"int", TOKEN_INT},       {"long", TOKEN_LONG},
                          {"signed", TOKEN_SIGNED}, {"unsigned", TOKEN_UNSIGNED}, {"sizeof", TOKEN_SIZEOF}, {"if", TOKEN_IF},
                          {"else", TOKEN_ELSE},     {"while", TOKEN_WHILE},       {"for", TOKEN_FOR},       {"return", TOKEN_RETURN}};
    while (*c != '\0') {
        size_t position = c - text;

        if (*c == '0' && *(c + 1) == 'b') {
            c += 2;
            list_add(tokens, token_new_integer(TOKEN_INTEGER, position, strtol(c, &c, 2)));
            continue;
        }
        if (*c == '0') {
            c++;
            list_add(tokens, token_new_integer(TOKEN_INTEGER, position, strtol(c, &c, 8)));
            continue;
        }
        if (*c == '0' && *(c + 1) == 'x') {
            c += 2;
            list_add(tokens, token_new_integer(TOKEN_INTEGER, position, strtol(c, &c, 16)));
            continue;
        }
        if (isdigit(*c)) {
            list_add(tokens, token_new_integer(TOKEN_INTEGER, position, strtol(c, &c, 10)));
            continue;
        }

        if (*c == '\'' && *(c + 2) == '\'') {
            c++;
            list_add(tokens, token_new_integer(TOKEN_CHARACTER, position, *c));
            c += 2;
            continue;
        }

        if (*c == '"') {
            c++;
            char *ptr = c;
            while (*c != '"') c++;
            size_t size = c - ptr;
            c++;

            char *string = malloc(size + 1);
            int32_t strpos = 0;
            for (size_t i = 0; i < size; i++) {
                if (ptr[i] == '\\') {
                    i++;
                    if (ptr[i] >= '0' && ptr[i] <= '7') {
                        int32_t num = ptr[i++] - '0';
                        if (ptr[i] >= '0' && ptr[i] <= '7') {
                            num = (num << 3) + (ptr[i++] - '0');
                            if (ptr[i] >= '0' && ptr[i] <= '7') {
                                num = (num << 3) + (ptr[i++] - '0');
                            }
                        }
                        string[strpos++] = num;
                    }
                    else if (ptr[i] == 'x') {
                        i++;
                        int32_t num = 0;
                        while (isxdigit(ptr[i])) {
                            int32_t part;
                            if (ptr[i] >= '0' && ptr[i] <= '9') part = ptr[i] - '0';
                            if (ptr[i] >= 'a' && ptr[i] <= 'f') part = ptr[i] - 'a' + 10;
                            if (ptr[i] >= 'A' && ptr[i] <= 'F') part = ptr[i] - 'A' + 10;
                            num = (num << 4) + part;
                            i++;
                        }
                        string[strpos++] = num;
                    }
                    else if (ptr[i] == 'a') string[strpos++] = '\a';
                    else if (ptr[i] == 'b') string[strpos++] = '\b';
                    else if (ptr[i] == 'e') string[strpos++] = 27;
                    else if (ptr[i] == 'f') string[strpos++] = '\f';
                    else if (ptr[i] == 'n') string[strpos++] = '\n';
                    else if (ptr[i] == 'r') string[strpos++] = '\r';
                    else if (ptr[i] == 't') string[strpos++] = '\t';
                    else if (ptr[i] == 'v') string[strpos++] = '\v';
                    else if (ptr[i] == '\\') string[strpos++] = '\\';
                    else if (ptr[i] == '\'') string[strpos++] = '\'';
                    else if (ptr[i] == '"') string[strpos++] = '"';
                    else if (ptr[i] == '?') string[strpos++] = '?';
                    else string[strpos++] = ptr[i];
                } else {
                    string[strpos++] = ptr[i];
                }
            }
            string[strpos] = '\0';
            list_add(tokens, token_new_string(TOKEN_STRING, position, string));
            continue;
        }

        if (isalpha(*c) || *c == '_') {
            char *ptr = c;
            while (isalnum(*c) || *c == '_') c++;
            size_t size = c - ptr;
            char *string = malloc(size + 1);
            memcpy(string, ptr, size);
            string[size] = '\0';

            bool found = false;
            for (size_t i = 0; i < sizeof(keywords) / sizeof(Keyword); i++) {
                Keyword *keyword = &keywords[i];
                if (!strcmp(string, keyword->keyword)) {
                    list_add(tokens, token_new(keyword->kind, position));
                    found = true;
                    break;
                }
            }
            if (!found) {
                list_add(tokens, token_new_string(TOKEN_VARIABLE, position, string));
            }
            continue;
        }

        if (*c == '(') {
            list_add(tokens, token_new(TOKEN_LPAREN, position));
            c++;
            continue;
        }
        if (*c == ')') {
            list_add(tokens, token_new(TOKEN_RPAREN, position));
            c++;
            continue;
        }
        if (*c == '{') {
            list_add(tokens, token_new(TOKEN_LCURLY, position));
            c++;
            continue;
        }
        if (*c == '}') {
            list_add(tokens, token_new(TOKEN_RCURLY, position));
            c++;
            continue;
        }
        if (*c == '[') {
            list_add(tokens, token_new(TOKEN_LBRACKET, position));
            c++;
            continue;
        }
        if (*c == ']') {
            list_add(tokens, token_new(TOKEN_RBRACKET, position));
            c++;
            continue;
        }
        if (*c == ',') {
            list_add(tokens, token_new(TOKEN_COMMA, position));
            c++;
            continue;
        }
        if (*c == ';') {
            list_add(tokens, token_new(TOKEN_SEMICOLON, position));
            c++;
            continue;
        }

        if (*c == '=') {
            if (*(c + 1) == '=') {
                list_add(tokens, token_new(TOKEN_EQ, position));
                c += 2;
                continue;
            }
            list_add(tokens, token_new(TOKEN_ASSIGN, position));
            c++;
            continue;
        }
        if (*c == '+') {
            list_add(tokens, token_new(TOKEN_ADD, position));
            c++;
            continue;
        }
        if (*c == '-') {
            list_add(tokens, token_new(TOKEN_SUB, position));
            c++;
            continue;
        }
        if (*c == '*') {
            list_add(tokens, token_new(TOKEN_STAR, position));
            c++;
            continue;
        }
        if (*c == '/') {
            list_add(tokens, token_new(TOKEN_DIV, position));
            c++;
            continue;
        }
        if (*c == '%') {
            list_add(tokens, token_new(TOKEN_MOD, position));
            c++;
            continue;
        }
        if (*c == '<') {
            if (*(c + 1) == '=') {
                list_add(tokens, token_new(TOKEN_LTEQ, position));
                c += 2;
                continue;
            }
            list_add(tokens, token_new(TOKEN_LT, position));
            c++;
            continue;
        }
        if (*c == '>') {
            if (*(c + 1) == '=') {
                list_add(tokens, token_new(TOKEN_GTEQ, position));
                c += 2;
                continue;
            }
            list_add(tokens, token_new(TOKEN_GT, position));
            c++;
            continue;
        }
        if (*c == '!') {
            if (*(c + 1) == '=') {
                list_add(tokens, token_new(TOKEN_NEQ, position));
                c += 2;
                continue;
            }
            list_add(tokens, token_new(TOKEN_LOGIC_NOT, position));
            c++;
            continue;
        }
        if (*c == '|' && *(c + 1) == '|') {
            list_add(tokens, token_new(TOKEN_LOGIC_OR, position));
            c += 2;
            continue;
        }
        if (*c == '&') {
            if (*(c + 1) == '&') {
                list_add(tokens, token_new(TOKEN_LOGIC_AND, position));
                c += 2;
                continue;
            }
            list_add(tokens, token_new(TOKEN_AND, position));
            c++;
            continue;
        }
        if (isspace(*c)) {
            c++;
            continue;
        }

        fprintf(stderr, "%s\n", text);
        for (size_t i = 0; i < position; i++) fprintf(stderr, " ");
        fprintf(stderr, "^\nUnexpected character: '%c'\n", *c);
        exit(EXIT_FAILURE);
    }
    list_add(tokens, token_new(TOKEN_EOF, c - text));
    return tokens;
}
