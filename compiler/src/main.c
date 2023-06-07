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

typedef struct File {
    char *path;
    FILE *file;
    char *text;
} File;

int main(int argc, char **argv) {
    // Print help text
    if (argc == 1) {
        printf("Bassie C Compiler\n");
        return EXIT_FAILURE;
    }

    // Parse arguments
    bool debug = false;
    Arch arch = ARCH_X86_64;
#ifdef __aarch64__
    arch = ARCH_ARM64;
#endif
    List files = {0};
    list_init(&files);
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

        if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--inline")) {
            i++;
            File *file = calloc(1, sizeof(File));
            file->path = "./inline";
            file->text = argv[i];
            list_add(&files, file);
            continue;
        }

        if (!strcmp(argv[i], "-")) {
            File *file = calloc(1, sizeof(File));
            file->path = "./stdin";
            file->file = stdin;
            list_add(&files, file);
        } else {
            File *file = calloc(1, sizeof(File));
            file->path = argv[i];
            file->file = fopen(argv[i], "rb");
            list_add(&files, file);
        }
    }

    // Create program
    Program program = {.arch = arch};
    list_init(&program.globals);
    list_init(&program.functions);

    // Read input files
    for (size_t i = 0; i < files.size; i++) {
        File *file = files.items[i];
        if (file->file != NULL) {
            file->text = file_read(file->file);
            fclose(file->file);
        }
    }

    // Process input files
    for (size_t i = 0; i < files.size; i++) {
        File *file = files.items[i];

        // Lexer
        size_t tokens_size;
        Token *tokens = lexer(file->path, file->text, &tokens_size);
        if (debug) {
            for (size_t i = 0; i < tokens_size; i++) {
                Token *token = &tokens[i];
                printf("%s ", token_kind_to_string(token->kind));
            }
            printf("\n");
        }

        // Parser
        parser(&program, tokens, tokens_size);
    }
    if (debug) {
        program_dump(stdout, &program);
    }

    // Codegen program
    program.text_section = section_new(4 * 1024);
    program.data_section = section_new(4 * 1024);
    codegen(&program);
    if (debug) {
        printf(".text:\n");
        section_dump(stdout, program.text_section);
        printf("\n.data:\n");
        section_dump(stdout, program.data_section);
    }

    // Execute program
    section_make_executable(program.text_section);
    return ((JitFunc)program.main_func)();
}
