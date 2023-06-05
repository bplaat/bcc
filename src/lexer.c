#include "lexer.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

// Source
Source *source_new(char *path, char *text) {
    Source *source = malloc(sizeof(Source));
    source->path = strdup(path);
    source->text = strdup(text);

    // Reverse loop over path to find basename
    char *c = source->path + strlen(source->path);
    while (*c != '/' && c != source->path) c--;
    source->basename = c + 1;

    source->dirname = strndup(source->path, source->basename - 1 - source->path);
    return source;
}

// Token
char *token_kind_to_string(TokenKind kind) {
    if (kind == TOKEN_EOF) return "EOF";
    if (kind == TOKEN_UNKNOWN) return "unknown?";

    if (kind == TOKEN_VARIABLE) return "variable";
    if (kind > TOKEN_INTEGER_BEGIN && kind < TOKEN_INTEGER_END) {
        if (kind == TOKEN_I8) return "character";
        return "integer";
    }

    if (kind == TOKEN_CHAR) return "char";
    if (kind == TOKEN_SHORT) return "short";
    if (kind == TOKEN_INT) return "int";
    if (kind == TOKEN_LONG) return "long";
    if (kind == TOKEN_SIGNED) return "signed";
    if (kind == TOKEN_UNSIGNED) return "unsigned";

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
    if (kind == TOKEN_LOGICAL_NOT) return "!";

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
static Keyword keywords[] = {{"char", TOKEN_CHAR},         {"short", TOKEN_SHORT},  {"int", TOKEN_INT},   {"long", TOKEN_LONG},   {"signed", TOKEN_SIGNED},
                             {"unsigned", TOKEN_UNSIGNED},

                             {"sizeof", TOKEN_SIZEOF},     {"if", TOKEN_IF},        {"else", TOKEN_ELSE}, {"while", TOKEN_WHILE}, {"do", TOKEN_DO},
                             {"for", TOKEN_FOR},           {"return", TOKEN_RETURN}};

static Keyword operators[] = {
    {"<<=", TOKEN_SHL_ASSIGN}, {">>=", TOKEN_SHR_ASSIGN},

    {"+=", TOKEN_ADD_ASSIGN},  {"-=", TOKEN_SUB_ASSIGN},  {"*=", TOKEN_MUL_ASSIGN}, {"/=", TOKEN_DIV_ASSIGN}, {"%=", TOKEN_MOD_ASSIGN},
    {"&=", TOKEN_AND_ASSIGN},  {"|=", TOKEN_OR_ASSIGN},   {"^=", TOKEN_XOR_ASSIGN}, {"<<", TOKEN_SHL},        {">>", TOKEN_SHR},
    {"==", TOKEN_EQ},          {"!=", TOKEN_NEQ},         {"<=", TOKEN_LTEQ},       {">=", TOKEN_GTEQ},       {"&&", TOKEN_LOGICAL_AND},
    {"||", TOKEN_LOGICAL_OR},

    {"(", TOKEN_LPAREN},       {")", TOKEN_RPAREN},       {"[", TOKEN_LBLOCK},      {"]", TOKEN_RBLOCK},      {"{", TOKEN_LCURLY},
    {"}", TOKEN_RCURLY},       {",", TOKEN_COMMA},        {"?", TOKEN_QUESTION},    {":", TOKEN_COLON},       {";", TOKEN_SEMICOLON},
    {"~", TOKEN_NOT},          {"!", TOKEN_LOGICAL_NOT},  {"=", TOKEN_ASSIGN},      {"+", TOKEN_ADD},         {"-", TOKEN_SUB},
    {"*", TOKEN_MUL},          {"/", TOKEN_DIV},          {"%", TOKEN_MOD},         {"&", TOKEN_AND},         {"|", TOKEN_OR},
    {"^", TOKEN_XOR},          {"<", TOKEN_LT},           {">", TOKEN_GT}};

static TokenKind lexer_integer_suffix(char *c, char **c_ptr) {
    if (*c == 'u' || *c == 'U') {
        c++;
        if (*c == 'l' || *c == 'L') {
            c++;
            if (*c == 'l' || *c == 'L') c++;
            *c_ptr = c;
            return TOKEN_U64;
        }
        *c_ptr = c;
        return TOKEN_U32;
    }
    if (*c == 'l' || *c == 'L') {
        c++;
        if (*c == 'l' || *c == 'L') c++;
        *c_ptr = c;
        return TOKEN_I64;
    }
    *c_ptr = c;
    return TOKEN_I32;
}

Token *lexer(char *path, char *text, size_t *tokens_size) {
    Source *source = source_new(path, text);

    size_t capacity = 1024;
    Token *tokens = malloc(capacity * sizeof(Token));
    size_t size = 0;

    char *c = text;
    char *line_start = c;
    int32_t line = 1;
    for (;;) {
        tokens[size].source = source;
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
            int64_t integer = strtol(c, &c, 2);
            tokens[size].kind = lexer_integer_suffix(c, &c);
            tokens[size++].integer = integer;
            continue;
        }
        if (*c == '0' && (isdigit(*(c + 1)) || *(c + 1) == 'o')) {
            if (*(c + 1) == 'o') c++;
            c++;
            int64_t integer = strtol(c, &c, 8);
            tokens[size].kind = lexer_integer_suffix(c, &c);
            tokens[size++].integer = integer;
            continue;
        }
        if (*c == '0' && *(c + 1) == 'x') {
            c += 2;
            int64_t integer = strtol(c, &c, 16);
            tokens[size].kind = lexer_integer_suffix(c, &c);
            tokens[size++].integer = integer;
            continue;
        }
        if (isdigit(*c)) {
            int64_t integer = strtol(c, &c, 10);
            tokens[size].kind = lexer_integer_suffix(c, &c);
            tokens[size++].integer = integer;
            continue;
        }

        // Characters
        if (*c == '\'') {
            c++;
            tokens[size].kind = TOKEN_I8;
            tokens[size++].integer = *c;
            c += 2;
            continue;
        }

        // Variables
        if (isalpha(*c) || *c == '_') {
            char *string = c;
            while (isalnum(*c) || *c == '_') c++;
            size_t string_size = c - string;

            bool keyword_found = false;
            for (size_t i = 0; i < sizeof(keywords) / sizeof(Keyword); i++) {
                Keyword *keyword = &keywords[i];
                size_t keyword_size = strlen(keyword->keyword);
                if (!memcmp(string, keyword->keyword, keyword_size) && string_size == keyword_size) {
                    tokens[size++].kind = keyword->kind;
                    keyword_found = true;
                    break;
                }
            }
            if (!keyword_found) {
                tokens[size].kind = TOKEN_VARIABLE;
                tokens[size++].variable = strndup(string, string_size);
            }
            continue;
        }

        // Operators
        bool operator_found = false;
        for (size_t i = 0; i < sizeof(operators) / sizeof(Keyword); i++) {
            Keyword *keyword = &operators[i];
            size_t keyword_size = strlen(keyword->keyword);
            if (!memcmp(c, keyword->keyword, keyword_size)) {
                tokens[size++].kind = keyword->kind;
                c += keyword_size;
                operator_found = true;
                break;
            }
        }
        if (operator_found) {
            continue;
        }

        // Unknown character
        tokens[size].kind = TOKEN_UNKNOWN;
        tokens[size++].integer = *c++;
    }
    *tokens_size = size;
    return tokens;
}
