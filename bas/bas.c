// ### Bassie Assembler ###
// I want to support
// Platforms: Windows (PE), Linux (ELF), macOS (MachO)
// Architectures: x86, x86_64, arm64
// Assembler in -> Binary object file out for linker

// gcc bas.c -o bas && ./bas test.s
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>

// ##############################################
// #################### UTILS ###################
// ##############################################

size_t align(size_t size, size_t alignment) {
    return (size + alignment - 1) / alignment * alignment;
}

char *strdup(const char *string) {
    char *newString = malloc(strlen(string) + 1);
    strcpy(newString, string);
    return newString;
}

char *format(char *fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    return strdup(buffer);
}

char *file_read(char *path) {
    FILE *file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer = malloc(fileSize + 1);
    fileSize = fread(buffer, 1, fileSize, file);
    buffer[fileSize] = '\0';
    fclose(file);
    return buffer;
}

typedef struct List {
    void **items;
    size_t capacity;
    size_t size;
} List;

#define list_get(list, index) ((list)->items[index])

List *list_new(size_t capacity) {
    List *list = malloc(sizeof(List));
    list->items = malloc(sizeof(void *) * capacity);
    list->capacity = capacity;
    list->size = 0;
    return list;
}

void list_add(List *list, void *item) {
    if (list->size == list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, sizeof(void *) * list->capacity);
    }
    list->items[list->size++] = item;
}

char *list_to_string(List *list) {
    size_t size = 0;
    for (size_t i = 0; i < list->size; i++) {
        size += strlen(list_get(list, i));
    }
    char *string = malloc(size + 1);
    string[0] = '\0';
    for (size_t i = 0; i < list->size; i++) {
        strcat(string, list_get(list, i));
    }
    return string;
}

List *split_lines(char *text) {
    List *lines = list_new(1024);
    char *c = text;
    list_add(lines, c);
    while (*c != '\0') {
        if (*c == '\n' || *c == '\r') {
            if (*c == '\r') {
                *c = '\0';
                c++;
            } else {
                *c = '\0';
            }
            c++;
            list_add(lines, c);
        } else {
            c++;
        }
    }
    return lines;
}

void print_error(char *path, List *lines, int32_t line, int32_t position, char *fmt, ...) {
    fprintf(stderr, "%s:%d:%d ERROR: ", path, line + 1, position + 1);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n%4d | %s\n", line + 1, list_get(lines, line));
    fprintf(stderr, "     | ");
    for (int32_t i = 0; i < position; i++) fprintf(stderr, " ");
    fprintf(stderr, "^\n");
    exit(EXIT_FAILURE);
}

// ##############################################
// #################### ARCH ####################
// ##############################################

typedef enum ArchKind {
    ARCH_X86_64
} ArchKind;

typedef struct Arch {
    ArchKind kind;
} Arch;

// ##############################################
// #################### LEXER ###################
// ##############################################

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

typedef struct Keyword {
    char *keyword;
    TokenKind kind;
} Keyword;

List *lexer(Arch *arch, char *path, List *lines) {
    List *tokens = list_new(1024);

    Keyword *keywords;
    size_t keywordsSize;
    if (arch->kind == ARCH_X86_64) {
        Keyword _keywords[] = {
            {"times", TOKEN_TIMES}, {"db", TOKEN_DB}, {"dw", TOKEN_DW}, {"dd", TOKEN_DD}, {"dq", TOKEN_DQ},

            {"byte", TOKEN_BYTE}, {"word", TOKEN_WORD}, {"dword", TOKEN_DWORD}, {"qword", TOKEN_QWORD}, {"ptr", TOKEN_PTR},

            {"eax", TOKEN_EAX}, {"ecx", TOKEN_ECX}, {"edx", TOKEN_EDX}, {"ebx", TOKEN_EBX},
            {"esp", TOKEN_ESP}, {"ebp", TOKEN_EBP}, {"esi", TOKEN_ESI}, {"edi", TOKEN_EDI},
            {"rax", TOKEN_RAX}, {"rcx", TOKEN_RCX}, {"rdx", TOKEN_RDX}, {"rbx", TOKEN_RBX},
            {"rsp", TOKEN_RSP}, {"rbp", TOKEN_RBP}, {"rsi", TOKEN_RSI}, {"rdi", TOKEN_RDI},

            {"nop", TOKEN_X86_64_NOP}, {"mov", TOKEN_X86_64_MOV}, {"lea", TOKEN_X86_64_LEA}, {"add", TOKEN_X86_64_ADD}, {"adc", TOKEN_X86_64_ADC},
            {"sub", TOKEN_X86_64_SUB}, {"sbb", TOKEN_X86_64_SBB}, {"cmp", TOKEN_X86_64_CMP},
            {"and", TOKEN_X86_64_AND}, {"or", TOKEN_X86_64_OR}, {"xor", TOKEN_X86_64_XOR}, {"neg", TOKEN_X86_64_NEG},
            {"push", TOKEN_X86_64_PUSH}, {"pop", TOKEN_X86_64_POP}, {"syscall", TOKEN_X86_64_SYSCALL},
            {"cdq", TOKEN_X86_64_CDQ}, {"cqo", TOKEN_X86_64_CQO}, {"leave", TOKEN_X86_64_LEAVE}, {"ret", TOKEN_X86_64_RET}
        };
        keywords = _keywords;
        keywordsSize = sizeof(_keywords) / sizeof(Keyword);
    }

    for (size_t line = 0; line < lines->size; line++) {
        char *text = list_get(lines, line);
        char *c = text;
        int32_t position = 0;
        while (*c != '\0') {
            position = c - text;

            if (*c == ';' || *c == '#' || (*c == '/' && *(c + 1) == '/')) {
                while (*c != '\0') c++;
                break;
            }

            if (*c == '0' && *(c + 1) == 'b') {
                c += 2;
                list_add(tokens, token_new_integer(TOKEN_INTEGER, line, position, strtol(c, &c, 2)));
                continue;
            }
            if (*c == '0' && *(c + 1) == 'x') {
                c += 2;
                list_add(tokens, token_new_integer(TOKEN_INTEGER, line, position, strtol(c, &c, 16)));
                continue;
            }
            if (*c == '0') {
                c++;
                list_add(tokens, token_new_integer(TOKEN_INTEGER, line, position, strtol(c, &c, 8)));
                continue;
            }
            if (isdigit(*c)) {
                list_add(tokens, token_new_integer(TOKEN_INTEGER, line, position, strtol(c, &c, 10)));
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
                for (size_t i = 0; i < keywordsSize; i++) {
                    Keyword *keyword = &keywords[i];
                    if (!strcmp(string, keyword->keyword)) {
                        list_add(tokens, token_new(keyword->kind, line, position));
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    list_add(tokens, token_new_string(TOKEN_KEYWORD, line, position, string));
                }
                continue;
            }

            if (*c == '"' || *c == '\'') {
                char start = *c;
                c++;
                char *ptr = c;
                while (*c != start) {
                    if (*c == '\0') {
                        print_error(path, lines, line, c - text,
                            "Missing terminating %c character", start);
                    }
                    c++;
                }
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
                        } else if (ptr[i] == 'x') {
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
                        } else if (ptr[i] == 'a')
                            string[strpos++] = '\a';
                        else if (ptr[i] == 'b')
                            string[strpos++] = '\b';
                        else if (ptr[i] == 'e')
                            string[strpos++] = 27;
                        else if (ptr[i] == 'f')
                            string[strpos++] = '\f';
                        else if (ptr[i] == 'n')
                            string[strpos++] = '\n';
                        else if (ptr[i] == 'r')
                            string[strpos++] = '\r';
                        else if (ptr[i] == 't')
                            string[strpos++] = '\t';
                        else if (ptr[i] == 'v')
                            string[strpos++] = '\v';
                        else if (ptr[i] == '\\')
                            string[strpos++] = '\\';
                        else if (ptr[i] == '\'')
                            string[strpos++] = '\'';
                        else if (ptr[i] == '"')
                            string[strpos++] = '"';
                        else if (ptr[i] == '?')
                            string[strpos++] = '?';
                        else
                            string[strpos++] = ptr[i];
                    } else {
                        string[strpos++] = ptr[i];
                    }
                }
                string[strpos] = '\0';
                list_add(tokens, token_new_string(TOKEN_STRING, line, position, string));
                continue;
            }

            if (*c == '(') {
                list_add(tokens, token_new(TOKEN_LPAREN, line, position));
                c++;
                continue;
            }
            if (*c == ')') {
                list_add(tokens, token_new(TOKEN_RPAREN, line, position));
                c++;
                continue;
            }
            if (*c == '[') {
                list_add(tokens, token_new(TOKEN_LBRACKET, line, position));
                c++;
                continue;
            }
            if (*c == ']') {
                list_add(tokens, token_new(TOKEN_RBRACKET, line, position));
                c++;
                continue;
            }
            if (*c == ':') {
                list_add(tokens, token_new(TOKEN_COLON, line, position));
                c++;
                continue;
            }
            if (*c == ',') {
                list_add(tokens, token_new(TOKEN_COMMA, line, position));
                c++;
                continue;
            }

            if (*c == '+') {
                list_add(tokens, token_new(TOKEN_ADD, line, position));
                c++;
                continue;
            }
            if (*c == '-') {
                list_add(tokens, token_new(TOKEN_SUB, line, position));
                c++;
                continue;
            }
            if (*c == '*') {
                list_add(tokens, token_new(TOKEN_MUL, line, position));
                c++;
                continue;
            }
            if (*c == '/') {
                list_add(tokens, token_new(TOKEN_DIV, line, position));
                c++;
                continue;
            }
            if (*c == '%') {
                list_add(tokens, token_new(TOKEN_MOD, line, position));
                c++;
                continue;
            }
            if (isspace(*c)) {
                c++;
                continue;
            }

            print_error(path, lines, line, position,
                "Unexpected character: '%c'", *c);
        }
        list_add(tokens, token_new(TOKEN_NEWLINE, line, position));
    }
    list_add(tokens, token_new(TOKEN_EOF, lines->size, 0));
    return tokens;
}

// ##############################################
// ################### PARSER ###################
// ##############################################

typedef enum NodeKind {
    NODE_PROGRAM,

    NODE_LABEL,
    NODE_INSTRUCTION,
    NODE_TIMES,

    NODE_INTEGER,
    NODE_STRING,
    NODE_KEYWORD,
    NODE_ADDR,

    NODE_NEG,
    NODE_ADD,
    NODE_SUB,
    NODE_MUL,
    NODE_DIV,
    NODE_MOD,

    NODE_REGISTER
} NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind kind;
    union {
        int64_t integer;
        char *string;
        struct {
            int32_t reg;
            int32_t size;
            Node *unary;
        };
        struct {
            Node *lhs;
            Node *rhs;
        };
        struct {
            TokenKind opcode;
            List *nodes;
        };
    };
};

Node *node_new(NodeKind kind) {
    Node *node = malloc(sizeof(Node));
    node->kind = kind;
    return node;
}

Node *node_new_integer(int64_t integer) {
    Node *node = node_new(NODE_INTEGER);
    node->integer = integer;
    return node;
}

Node *node_new_string(NodeKind kind, char *string) {
    Node *node = node_new(kind);
    node->string = string;
    return node;
}

Node *node_new_unary(NodeKind kind, Node *unary) {
    Node *node = node_new(kind);
    node->unary = unary;
    return node;
}

Node *node_new_operation(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = node_new(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *node_new_multiple(NodeKind kind) {
    Node *node = node_new(kind);
    node->nodes = list_new(8);
    return node;
}

Node *node_new_register(int32_t reg, int32_t size) {
    Node *node = node_new(NODE_REGISTER);
    node->reg = reg;
    node->size = size;
    return node;
}

char *node_to_string(Node *node) {
    if (node->kind == NODE_PROGRAM) {
        List *sb = list_new(8);
        for (size_t i = 0; i < node->nodes->size; i++) {
            list_add(sb, node_to_string(list_get(node->nodes, i)));
            list_add(sb, "\n");
        }
        return list_to_string(sb);
    }
    if (node->kind == NODE_LABEL) {
        return format("%s:", node->string);
    }
    if (node->kind == NODE_INSTRUCTION) {
        List *sb = list_new(8);
        list_add(sb, token_kind_to_string(node->opcode));
        list_add(sb, " ");
        for (size_t i = 0; i < node->nodes->size; i++) {
            list_add(sb, node_to_string(list_get(node->nodes, i)));
            if (i != node->nodes->size - 1) list_add(sb, ", ");
        }
        return list_to_string(sb);
    }
    if (node->kind == NODE_TIMES) {
        List *sb = list_new(8);
        list_add(sb, "times ");
        list_add(sb, node_to_string(node->lhs));
        list_add(sb, " ");
        list_add(sb, node_to_string(node->rhs));
        return list_to_string(sb);
    }

    if (node->kind == NODE_INTEGER) {
        return format("%lld", node->integer);
    }
    if (node->kind == NODE_STRING) {
        List *sb = list_new(8);
        list_add(sb, "\"");
        list_add(sb, node->string);
        list_add(sb, "\"");
        return list_to_string(sb);
    }
    if (node->kind == NODE_KEYWORD) {
        return strdup(node->string);
    }
    if (node->kind == NODE_ADDR) {
        List *sb = list_new(8);
        if (node->size == 8) list_add(sb, "byte");
        if (node->size == 16) list_add(sb, "word");
        if (node->size == 32) list_add(sb, "dword");
        if (node->size == 64) list_add(sb, "qword");
        list_add(sb, " ptr [");
        list_add(sb, node_to_string(node->unary));
        list_add(sb, "]");
        return list_to_string(sb);
    }

    if (node->kind == NODE_NEG) {
        List *sb = list_new(8);
        if (node->kind == NODE_NEG) list_add(sb, "(- ");
        list_add(sb, node_to_string(node->unary));
        list_add(sb, ")");
        return list_to_string(sb);
    }

    if (node->kind >= NODE_ADD && node->kind <= NODE_MOD) {
        List *sb = list_new(8);
        list_add(sb, "(");
        list_add(sb, node_to_string(node->lhs));
        if (node->kind == NODE_ADD) list_add(sb, " + ");
        if (node->kind == NODE_SUB) list_add(sb, " - ");
        if (node->kind == NODE_MUL) list_add(sb, " * ");
        if (node->kind == NODE_DIV) list_add(sb, " / ");
        if (node->kind == NODE_MOD) list_add(sb, " %% ");
        list_add(sb, node_to_string(node->rhs));
        list_add(sb, ")");
        return list_to_string(sb);
    }

    if (node->kind == NODE_REGISTER) {
        if (node->size == 32) {
            if (node->reg == 0) return strdup("eax");
            if (node->reg == 1) return strdup("ecx");
            if (node->reg == 2) return strdup("edx");
            if (node->reg == 3) return strdup("ebx");
            if (node->reg == 4) return strdup("esp");
            if (node->reg == 5) return strdup("ebp");
            if (node->reg == 6) return strdup("esi");
            if (node->reg == 7) return strdup("edi");
        }
        if (node->size == 64) {
            if (node->reg == 0) return strdup("rax");
            if (node->reg == 1) return strdup("rcx");
            if (node->reg == 2) return strdup("rdx");
            if (node->reg == 3) return strdup("rbx");
            if (node->reg == 4) return strdup("rsp");
            if (node->reg == 5) return strdup("rbp");
            if (node->reg == 6) return strdup("rsi");
            if (node->reg == 7) return strdup("rdi");
        }
    }
    return NULL;
}

typedef struct Parser {
    Arch *arch;
    char *path;
    List *lines;
    List *tokens;
    int32_t position;
} Parser;

Node *parser_program(Parser *parser);
Node *parser_statement(Parser *parser);
Node *parser_add(Parser *parser);
Node *parser_mul(Parser *parser);
Node *parser_unary(Parser *parser);
Node *parser_primary(Parser *parser);

Node *parser(Arch *arch, char *path, List *lines, List *tokens) {
    Parser parser;
    parser.arch = arch;
    parser.path = path;
    parser.lines = lines;
    parser.tokens = tokens;
    parser.position = 0;
    return parser_program(&parser);
}

#define current() ((Token *)list_get(parser->tokens, parser->position))
#define next(pos) ((Token *)list_get(parser->tokens, parser->position + 1 + pos))

void parser_eat(Parser *parser, TokenKind kind) {
    if (current()->kind == kind) {
        parser->position++;
    } else {
        print_error(parser->path, parser->lines, current()->line, current()->position,
            "Unexpected: '%s' needed '%s'", token_kind_to_string(current()->kind), token_kind_to_string(kind));
    }
}

Node *parser_program(Parser *parser) {
    Node *programNode = node_new_multiple(NODE_PROGRAM);
    while (current()->kind != TOKEN_EOF) {
        Node *node = parser_statement(parser);
        if (node != NULL) list_add(programNode->nodes, node);
    }
    return programNode;
}

Node *parser_statement(Parser *parser) {

    if (current()->kind == TOKEN_TIMES) {
        parser_eat(parser, TOKEN_TIMES);
        Node *lhs = parser_add(parser);
        return node_new_operation(NODE_TIMES, lhs, parser_statement(parser));
    }


    if (current()->kind == TOKEN_NEWLINE) {
        parser_eat(parser, TOKEN_NEWLINE);
        return NULL;
    }

    if (current()->kind == TOKEN_KEYWORD && next(0)->kind == TOKEN_COLON) {
        char *name = current()->string;
        parser_eat(parser, TOKEN_KEYWORD);
        parser_eat(parser, TOKEN_COLON);
        Node *node = node_new_string(NODE_LABEL, name);
        if (current()->kind == TOKEN_NEWLINE) {
            parser_eat(parser, TOKEN_NEWLINE);
        }
        return node;
    }

    if (
        (current()->kind >= TOKEN_TIMES && current()->kind <= TOKEN_DQ) ||
        (current()->kind >= TOKEN_X86_64_NOP && current()->kind <= TOKEN_X86_64_RET)
    ) {
        Node *instructionNode = node_new_multiple(NODE_INSTRUCTION);
        instructionNode->opcode = current()->kind;
        parser_eat(parser, current()->kind);
        while (current()->kind != TOKEN_NEWLINE) {
            Node *node = parser_add(parser);
            if (node != NULL) list_add(instructionNode->nodes, node);
            if (current()->kind == TOKEN_COMMA) {
                parser_eat(parser, TOKEN_COMMA);
                if (current()->kind == TOKEN_NEWLINE) {
                    parser_eat(parser, TOKEN_NEWLINE);
                }
            } else {
                break;
            }
        }
        parser_eat(parser, TOKEN_NEWLINE);
        return instructionNode;
    }

    Node *node = parser_add(parser);
    parser_eat(parser, TOKEN_NEWLINE);
    return node;
}

Node *parser_add(Parser *parser) {
    Node *node = parser_mul(parser);
    while (current()->kind == TOKEN_ADD || current()->kind == TOKEN_SUB) {
        if (current()->kind == TOKEN_ADD) {
            parser_eat(parser, TOKEN_ADD);
            node = node_new_operation(NODE_ADD, node, parser_mul(parser));
        }

        if (current()->kind == TOKEN_SUB) {
            parser_eat(parser, TOKEN_SUB);
            node = node_new_operation(NODE_SUB, node, parser_mul(parser));
        }
    }
    return node;
}

Node *parser_mul(Parser *parser) {
    Node *node = parser_unary(parser);
    while (current()->kind == TOKEN_MUL || current()->kind == TOKEN_DIV || current()->kind == TOKEN_MOD) {
        if (current()->kind == TOKEN_MUL) {
            parser_eat(parser, TOKEN_MUL);
            node = node_new_operation(NODE_MUL, node, parser_unary(parser));
        }
        if (current()->kind == TOKEN_DIV) {
            parser_eat(parser, TOKEN_DIV);
            node = node_new_operation(NODE_DIV, node, parser_unary(parser));
        }
        if (current()->kind == TOKEN_MOD) {
            parser_eat(parser, TOKEN_MOD);
            node = node_new_operation(NODE_MOD, node, parser_unary(parser));
        }
    }
    return node;
}

Node *parser_unary(Parser *parser) {
    if (current()->kind == TOKEN_ADD) {
        parser_eat(parser, TOKEN_ADD);
        return parser_unary(parser);
    }
    if (current()->kind == TOKEN_SUB) {
        parser_eat(parser, TOKEN_SUB);
        Node *node = parser_unary(parser);
        if (node->kind == NODE_INTEGER) {
            node->integer = -node->integer;
            return node;
        }
        return node_new_unary(NODE_NEG, node);
    }
    return parser_primary(parser);
}

Node *parser_primary(Parser *parser) {
    if (current()->kind == TOKEN_LPAREN) {
        parser_eat(parser, TOKEN_LPAREN);
        Node *node = parser_add(parser);
        parser_eat(parser, TOKEN_RPAREN);
        return node;
    }
    if (current()->kind == TOKEN_INTEGER) {
        Node *node = node_new_integer(current()->integer);
        parser_eat(parser, TOKEN_INTEGER);
        return node;
    }
    if (current()->kind == TOKEN_STRING) {
        Node *node = node_new_string(NODE_STRING, current()->string);
        parser_eat(parser, TOKEN_STRING);
        return node;
    }

    if (
        (current()->kind >= TOKEN_BYTE && current()->kind <= TOKEN_QWORD) ||
        current()->kind == TOKEN_LBRACKET
    ) {
        size_t size = 0;
        if (current()->kind == TOKEN_BYTE) {
            size = 8;
            parser_eat(parser, TOKEN_BYTE);
        }
        else if (current()->kind == TOKEN_WORD) {
            size = 16;
            parser_eat(parser, TOKEN_WORD);
        }
        else if (current()->kind == TOKEN_DWORD) {
            size = 32;
            parser_eat(parser, TOKEN_DWORD);
        }
        else if (current()->kind == TOKEN_QWORD) {
            size = 64;
            parser_eat(parser, TOKEN_QWORD);
        }
        if (current()->kind == TOKEN_PTR) {
            parser_eat(parser, TOKEN_PTR);
        }

        parser_eat(parser, TOKEN_LBRACKET);
        Node *node = node_new_unary(NODE_ADDR, parser_add(parser));
        node->size = size;
        parser_eat(parser, TOKEN_RBRACKET);
        return node;
    }

    if (current()->kind >= TOKEN_EAX && current()->kind <= TOKEN_RDI) {
        Node *node;
        if (current()->kind >= TOKEN_EAX && current()->kind <= TOKEN_EDI) {
            node = node_new_register(current()->kind - TOKEN_EAX, 32);
        }
        if (current()->kind >= TOKEN_RAX && current()->kind <= TOKEN_RDI) {
            node = node_new_register(current()->kind - TOKEN_RAX, 64);
        }
        parser_eat(parser, current()->kind);
        return node;
    }

    if (current()->kind == TOKEN_KEYWORD) {
        Node *node = node_new_string(NODE_KEYWORD, current()->string);
        parser_eat(parser, TOKEN_KEYWORD);
        return node;
    }

    print_error(parser->path, parser->lines, current()->line, current()->position,
        "Unexpected: '%s'", token_kind_to_string(current()->kind));
}

// ##############################################
// ################### CODEGEN ##################
// ##############################################

void IMM16(uint8_t **end, int16_t imm) {
    int16_t *c = (int16_t *)*end;
    *c++ = imm;
    *end = (uint8_t *)c;
}

void IMM32(uint8_t **end, int32_t imm) {
    int32_t *c = (int32_t *)*end;
    *c++ = imm;
    *end = (uint8_t *)c;
}

void IMM64(uint8_t **end, int64_t imm) {
    int64_t *c = (int64_t *)*end;
    *c++ = imm;
    *end = (uint8_t *)c;
}

int64_t node_calc(Node *node) {
    if (node->kind == NODE_INTEGER) {
        return node->integer;
    }
    if (node->kind == NODE_ADD) {
        return node_calc(node->lhs) + node_calc(node->rhs);
    }
    if (node->kind == NODE_SUB) {
        return node_calc(node->lhs) - node_calc(node->rhs);
    }
    if (node->kind == NODE_MUL) {
        return node_calc(node->lhs) * node_calc(node->rhs);
    }
    if (node->kind == NODE_DIV) {
        return node_calc(node->lhs) / node_calc(node->rhs);
    }
    if (node->kind == NODE_MOD) {
        return node_calc(node->lhs) % node_calc(node->rhs);
    }
    return 0;
}

void node_write(Arch *arch, uint8_t **end, Node *node) {
    uint8_t *c = *end;

    if (node->kind == NODE_PROGRAM) {
        for (size_t i = 0; i < node->nodes->size; i++) {
            node_write(arch, &c, list_get(node->nodes, i));
        }
    }

    if (node->kind == NODE_TIMES) {
        for (int64_t i = 0; i < node->lhs->integer; i++) {
            node_write(arch, &c, node->rhs);
        }
    }

    if (node->kind == NODE_INSTRUCTION) {
        uint64_t zero = 0;
        if (node->opcode >= TOKEN_DB && node->opcode <= TOKEN_DQ) {
            size_t size = 1;
            if (node->opcode == TOKEN_DW) size = 2;
            if (node->opcode == TOKEN_DD) size = 4;
            if (node->opcode == TOKEN_DQ) size = 8;
            for (size_t i = 0; i < node->nodes->size; i++) {
                Node *child = list_get(node->nodes, i);
                if (child->kind == NODE_STRING) {
                    for (size_t j = 0; j < strlen(child->string); j++) {
                        *c++ = child->string[j];
                        if (size != 1) c += size - 1;
                    }
                } else {
                    int64_t answer = node_calc(child);
                    for (size_t j = 0; j < size; j++) {
                        *c++ = ((uint8_t *)&answer)[j];
                    }
                }
            }
        }

        if (node->opcode == TOKEN_X86_64_NOP) {
            *c++ = 0x90;
        }

        if (node->opcode >= TOKEN_X86_64_MOV && node->opcode <= TOKEN_X86_64_XOR) {
            Node *dest = (Node *)list_get(node->nodes, 0);
            Node *src = (Node *)list_get(node->nodes, 1);

            int32_t opcode, opcode2;
            if (node->opcode == TOKEN_X86_64_MOV) opcode = 0x89, opcode2 = 0b000;
            if (node->opcode == TOKEN_X86_64_LEA) opcode = 0x8d, opcode2 = 0b000;
            if (node->opcode == TOKEN_X86_64_ADD) opcode = 0x01, opcode2 = 0b000;
            if (node->opcode == TOKEN_X86_64_ADC) opcode = 0x11, opcode2 = 0b010;
            if (node->opcode == TOKEN_X86_64_SUB) opcode = 0x29, opcode2 = 0b101;
            if (node->opcode == TOKEN_X86_64_SBB) opcode = 0x19, opcode2 = 0b011;
            if (node->opcode == TOKEN_X86_64_CMP) opcode = 0x39, opcode2 = 0b111;
            if (node->opcode == TOKEN_X86_64_AND) opcode = 0x21, opcode2 = 0b100;
            if (node->opcode == TOKEN_X86_64_OR) opcode = 0x09, opcode2 = 0b001;
            if (node->opcode == TOKEN_X86_64_XOR) opcode = 0x31, opcode2 = 0b110;
            if (dest->size == 64) *c++ = 0x48;
            if (dest->kind == NODE_REGISTER) {
                if (src->kind == NODE_REGISTER) {
                    *c++ = opcode;
                    *c++ = (0b11 << 6) | (src->reg << 3) | dest->reg;
                }

                if (src->kind == NODE_ADDR) {
                    if (dest->size != src->size) {
                        fprintf(stderr, "ERROR addr size and dest reg are not the same\n");
                        exit(EXIT_FAILURE);
                    }
                    if (src->unary->kind != NODE_ADD && src->unary->kind != NODE_SUB) {
                        fprintf(stderr, "ERROR addr must contain add or sub\n");
                        exit(EXIT_FAILURE);
                    }

                    Node *srcreg = src->unary->lhs;
                    int64_t disp = src->unary->rhs->integer;
                    if (src->unary->kind == NODE_SUB) disp = -disp;
                    if (srcreg->size == 32) {
                        fprintf(stderr, "ERROR addr reg not 64\n");
                        exit(EXIT_FAILURE);
                    }

                    *c++ = node->opcode == TOKEN_X86_64_LEA ? opcode : (opcode | 0b10);
                    if (disp >= -128 && disp <= 127) {
                        *c++ = (0b01 << 6) | (dest->reg << 3) | srcreg->reg;
                        *c++ = disp;
                    } else {
                        *c++ = (0b10 << 6) | (dest->reg << 3) | srcreg->reg;
                        IMM32(&c, disp);
                    }
                }

                if (src->kind == NODE_INTEGER) {
                    if (node->opcode == TOKEN_X86_64_MOV) {
                        *c++ = 0xb8 + dest->reg;
                        if (dest->size == 64) {
                            IMM64(&c, src->integer);
                        } else {
                            IMM32(&c, src->integer);
                        }
                    } else {
                        *c++ = src->integer >= -128 && src->integer <= 127 ? 0x83 : 0x81;
                        *c++ = (0b11 << 6) | (opcode2 << 3) | dest->reg;
                        if (src->integer >= -128 && src->integer <= 127) {
                            *c++ = src->integer;
                        } else {
                            IMM32(&c, src->integer);
                        }
                    }
                }
            }
            if (dest->kind == NODE_ADDR) {
                if (dest->unary->kind != NODE_ADD && dest->unary->kind != NODE_SUB) {
                    fprintf(stderr, "ERROR addr must contain add or sub\n");
                    exit(EXIT_FAILURE);
                }

                Node *destreg = dest->unary->lhs;
                int64_t disp = dest->unary->rhs->integer;
                if (dest->unary->kind == NODE_SUB) disp = -disp;
                if (destreg->size == 32) {
                    fprintf(stderr, "ERROR addr reg not 64\n");
                    exit(EXIT_FAILURE);
                }

                if (src->kind == NODE_REGISTER) {
                    if (dest->size != src->size) {
                        fprintf(stderr, "ERROR addr size and dest reg are not the same\n");
                        exit(EXIT_FAILURE);
                    }

                    *c++ = opcode;
                    if (disp >= -128 && disp <= 127) {
                        *c++ = (0b01 << 6) | (src->reg << 3) | destreg->reg;
                        *c++ = disp;
                    } else {
                        *c++ = (0b10 << 6) | (src->reg << 3) | destreg->reg;
                        IMM32(&c, disp);
                    }
                }
                if (src->kind == NODE_INTEGER) {
                    *c++ = (src->integer >= -128 && src->integer <= 127) ? (node->opcode == TOKEN_X86_64_MOV ? 0xc6 : 0x83) : (node->opcode == TOKEN_X86_64_MOV ? 0xc7 : 0x81);
                    if (disp >= -128 && disp <= 127) {
                        *c++ = (0b01 << 6) | (opcode2 << 3) | destreg->reg;
                        *c++ = disp;
                    } else {
                        *c++ = (0b10 << 6) | (opcode2 << 3) | destreg->reg;
                        IMM32(&c, disp);
                    }
                    if (src->integer >= -128 && src->integer <= 127) {
                        *c++ = src->integer;
                    } else {
                        IMM32(&c, src->integer);
                    }
                }
            }
        }

        if (node->opcode == TOKEN_X86_64_NEG) {
            Node *dest = (Node *)list_get(node->nodes, 0);
            if (dest->size == 64) *c++ = 0x48;

            if (dest->kind == NODE_REGISTER) {
                *c++ = 0xf7;
                *c++ = (0b11 << 6) | (0b011 << 3) | dest->reg;
            }

            if (dest->kind == NODE_ADDR) {
                if (dest->unary->kind != NODE_ADD && dest->unary->kind != NODE_SUB) {
                    fprintf(stderr, "ERROR addr must contain add or sub\n");
                    exit(EXIT_FAILURE);
                }

                Node *destreg = dest->unary->lhs;
                int64_t disp = dest->unary->rhs->integer;
                if (dest->unary->kind == NODE_SUB) disp = -disp;
                if (destreg->size == 32) {
                    fprintf(stderr, "ERROR addr reg not 64\n");
                    exit(EXIT_FAILURE);
                }

                *c++ = 0xf7;
                if (disp >= -128 && disp <= 127) {
                    *c++ = (0b01 << 6) | (0b011 << 3) | destreg->reg;
                    *c++ = disp;
                } else {
                    *c++ = (0b10 << 6) | (0b011 << 3) | destreg->reg;
                    IMM32(&c, disp);
                }
            }
        }

        if (node->opcode == TOKEN_X86_64_PUSH) {
            Node *src = (Node *)list_get(node->nodes, 0);
            if (src->kind == NODE_REGISTER) {
                if (src->size == 32) {
                    fprintf(stderr, "ERROR push not 64\n");
                    exit(EXIT_FAILURE);
                }

                *c++ = 0x50 + src->reg;
            }
        }

        if (node->opcode == TOKEN_X86_64_POP) {
            Node *dest = (Node *)list_get(node->nodes, 0);
            if (dest->kind == NODE_REGISTER) {
                if (dest->size == 32) {
                    fprintf(stderr, "ERROR push not 64\n");
                    exit(EXIT_FAILURE);
                }

                *c++ = 0x58 + dest->reg;
            }
        }

        if (node->opcode == TOKEN_X86_64_SYSCALL) {
            *c++ = 0x0f;
            *c++ = 0x05;
        }

        if (node->opcode == TOKEN_X86_64_CDQ) {
            *c++ = 0x99;
        }

        if (node->opcode == TOKEN_X86_64_CQO) {
            *c++ = 0x48;
            *c++ = 0x99;
        }

        if (node->opcode == TOKEN_X86_64_LEAVE) {
            *c++ = 0xc9;
        }

        if (node->opcode == TOKEN_X86_64_RET) {
            if (node->nodes->size > 0) {
                Node *imm = (Node *)list_get(node->nodes, 0);
                *c++ = 0xc2;
                IMM16(&c, imm->integer);
            } else {
                *c++ = 0xc3;
            }
        }
    }

    *end = c;
}

// ##############################################
// #################### MAIN ####################
// ##############################################

int main(int argc, char **argv) {
    Arch x86_64 = { ARCH_X86_64 };

    char *path = argv[1];
    char *text = file_read(path);
    if (text == NULL) return EXIT_FAILURE;

    List *lines = split_lines(text);
    List *tokens = lexer(&x86_64, path, lines);
    Node *node = parser(&x86_64, path, lines, tokens);
    printf("%s\n", node_to_string(node));

    uint8_t *buffer = calloc(1024, 1);
    uint8_t *end = buffer;
    node_write(&x86_64, &end, node);

    FILE *f = fopen("test.bin", "wb");
    fwrite(buffer, 1, end - buffer, f);
    fclose(f);

    system("hexdump -H test.bin");
    printf("\n");
    system("ndisasm -b 64 test.bin");
    return EXIT_SUCCESS;
}
