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
    char *path = NULL;
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
            path = "stdin";
            file = stdin;
        } else {
            path = argv[i];
            file = fopen(argv[i], "rb");
        }
    }

    // Read input
    if (file == NULL) return EXIT_FAILURE;
    char *text = file_read(file);
    fclose(file);

    // Create program
    Program program = {.arch = arch};
    list_init(&program.functions);

    // Lexer
    size_t tokens_size;
    Token *tokens = lexer(path, text, &tokens_size);
    if (debug) {
        for (size_t i = 0; i < tokens_size; i++) {
            Token *token = &tokens[i];
            printf("%s ", token_kind_to_string(token->kind));
        }
        printf("\n\n");
    }

    // Parser
    parser(&program, tokens, tokens_size);
    if (debug) {
        program_dump(stdout, &program);
    }

    // Codegen
    program.section_text = page_new(4 * 1024);
    codegen(&program);
    if (debug) {
        printf(".text:\n");
        uint8_t *section_text = program.section_text->data;
        size_t i = 0;
        for (size_t y = 0; y < 128; y += 16) {
            for (int32_t x = 0; x < 16; x++) {
                printf("%02x ", section_text[i++]);
            }
            printf("\n");
        }
    }

    // Execute
    page_make_executable(program.section_text);
    return ((JitFunc)program.main_func)();
}
