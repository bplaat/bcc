#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "lexer.h"
#include "object.h"
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
            path = "./stdin";
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
    program.text_section = section_new(4 * 1024);
    program.data_section = section_new(4 * 1024);
    codegen(&program);
    if (debug) {
        printf(".text:\n");
        section_dump(stdout, program.text_section);
        printf("\n.data:\n");
        section_dump(stdout, program.data_section);
    }

    // Execute
    section_make_executable(program.text_section);
    return ((JitFunc)program.main_func)();
}
