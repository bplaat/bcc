#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "lexer.h"
#include "page.h"
#include "parser.h"
#include "utils.h"

typedef int64_t (*JitFunc)(void);

int main(int argc, char **argv) {
    // Print help text
    if (argc == 1) {
        printf("Bassie C Compiler\n");
        return EXIT_FAILURE;
    }

    // Parse arguments
    FILE *file = NULL;
    bool debug = false;
#ifdef __x86_64__
    Arch arch = ARCH_X86_64;
#endif
#ifdef __aarch64__
    Arch arch = ARCH_ARM64;
#endif
    for (int32_t i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--debug")) {
            debug = true;
            continue;
        }

        if (!strcmp(argv[i], "-a") || !strcmp(argv[i], "--arch")) {
            i++;
            if (!strcmp(argv[i], "x86_64")) {
                arch = ARCH_X86_64;
            }
            if (!strcmp(argv[i], "arm64")) {
                arch = ARCH_ARM64;
            }
            continue;
        }

        if (!strcmp(argv[i], "-")) {
            file = stdin;
        } else {
            file = fopen(argv[i], "rb");
        }
    }

    // Read input
    char *text = file_read(file);
    fclose(file);

    // Allocate code page
    Page *code = page_new(4 * 1024);

    // Lexer
    size_t tokens_size;
    Token *tokens = lexer(text, &tokens_size);
    if (debug) {
        for (size_t i = 0; i < tokens_size; i++) {
            Token *token = &tokens[i];
            printf("%s ", token_type_to_string(token->type));
        }
        printf("\n\n");
    }

    // Parser
    Node *node = parser(text, tokens, tokens_size);
    if (debug) {
        node_dump(stdout, node);
        printf("\n");
    }

    // Codegen
    codegen(arch, code->data, node);
    // if (debug) {
    //     size_t i = 0;
    //     for (size_t y = 0; y < 512; y += 16) {
    //         for (int32_t x = 0; x < 16; x++) {
    //             printf("%02x ", ((uint8_t *)code->data)[i++]);
    //         }
    //         printf("\n");
    //     }
    // }

    // Execute
    page_make_executable(code);
    return ((JitFunc)code->data)();
}
