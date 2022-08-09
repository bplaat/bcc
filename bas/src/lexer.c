#include "lexer.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "utils.h"

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
