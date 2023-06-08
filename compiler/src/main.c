#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen/codegen.h"
#include "lexer.h"
#include "object/object.h"
#include "object/section.h"
#include "parser.h"
#include "utils/utils.h"

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
    bool run = false;
    bool debug = false;
    char *output = "a.out";
    List files = {0};
    list_init(&files);

    System system = SYSTEM_LINUX;
#ifdef __APPLE__
    system = SYSTEM_MACOS;
#endif
#ifdef _WIN32
    system = SYSTEM_WINDOWS;
#endif

    Arch arch = ARCH_X86_64;
#ifdef __aarch64__
    arch = ARCH_ARM64;
#endif

    for (int32_t i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--run")) {
            run = true;
            continue;
        }

        if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--debug")) {
            debug = true;
            continue;
        }

        if (!strcmp(argv[i], "--os")) {
            i++;
            if (!strcmp(argv[i], "linux")) {
                system = SYSTEM_LINUX;
            }
            if (!strcmp(argv[i], "macos")) {
                system = SYSTEM_MACOS;
            }
            if (!strcmp(argv[i], "windows")) {
                system = SYSTEM_WINDOWS;
            }
            continue;
        }

        if (!strcmp(argv[i], "--arch")) {
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

        if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output")) {
            i++;
            output = argv[i];
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
    Program program = {
        .system = system,
        .arch = arch,
    };
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
    codegen(run, &program);

    // Run program
    if (run) {
        section_make_executable(program.text_section);
        return ((JitFunc)program.main_func)();
    }

    // Output object file
    object_out(output, system, &program);
    return EXIT_SUCCESS;
}
