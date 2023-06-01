#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "page.h"
#include "util.h"
#include "codegen.h"

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
    Page *code = page_new(16 * 1024);

    // Lexer
    size_t tokens_size;
    Token *tokens = lexer(text, &tokens_size);

    // Parser
    Node *node = parser(tokens, tokens_size);
    if (debug) {
        node_dump(stdout, node);
        printf("\n");
    }

    // Codegen
    codegen(arch, code->data, node);

    // Execute
    page_make_executable(code);
    return ((JitFunc)code->data)();
}
