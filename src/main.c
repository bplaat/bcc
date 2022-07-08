#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "lexer.h"
#include "parser.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Bassie C Compiler\n");
        return EXIT_SUCCESS;
    }

    // Parse arguments
    char *text = NULL;
    char *outputPath = "a";
    List *objects = list_new(4);
    bool dumpNode = false;

    Arch arch;
    arch.kind = ARCH_ARM64;
    arch.stackAlign = 16;
    arch.argumentRegs = (char *[]){"x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7"};

    int32_t position = 1;
    while (position < argc) {
        if (!strcmp(argv[position], "-o")) {
            position++;
            outputPath = argv[position++];
            continue;
        }

        if (!strcmp(argv[position], "-d")) {
            position++;
            dumpNode = true;
            continue;
        }

        if (!strcmp(argv[position], "-arch")) {
            position++;
            if (!strcmp(argv[position], "x86_64")) {
                position++;
                arch.kind = ARCH_X86_64;
                arch.stackAlign = 8;
                arch.argumentRegs = (char *[]){"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
            }
            continue;
        }

        if (text == NULL) {
            text = argv[position++];
        } else {
            list_add(objects, argv[position++]);
        }
    }

    // Compile program and write assembly file
    List *tokens = lexer(text);
    Node *node = parser(text, tokens, &arch);
    if (dumpNode) {
        node_print(stdout, node);
        return EXIT_SUCCESS;
    }

    char assemblyPath[512];
    strcpy(assemblyPath, outputPath);
    strcat(assemblyPath, ".s");
    FILE *assembly = fopen(assemblyPath, "w");

    if (arch.kind == ARCH_ARM64) {
        fprintf(assembly, "    .align 4\n");
        fprintf(assembly, "    .global _start\n");
    }
    if (arch.kind == ARCH_X86_64) {
        fprintf(assembly, "    global _start\n");
    }

    fprintf(assembly, "_start:\n");
    if (arch.kind == ARCH_ARM64) {
        fprintf(assembly, "    bl _main\n");
        fprintf(assembly, "    mov x16, #1\n");
        fprintf(assembly, "    svc #0x80\n\n");
    }
    if (arch.kind == ARCH_X86_64) {
        fprintf(assembly, "    call _main\n");
        fprintf(assembly, "    mov edi, eax\n");
        fprintf(assembly, "    mov eax, 0x2000001\n");
        fprintf(assembly, "    syscall\n\n");
    }

    codegen(assembly, node, &arch);
    fclose(assembly);

    // Assemble assembly file and link executable
    char objectPath[512];
    strcpy(objectPath, outputPath);
    strcat(objectPath, ".o");
    list_add(objects, objectPath);

    if (arch.kind == ARCH_ARM64) {
        char command[512];
        strcpy(command, "as ");
        strcat(command, assemblyPath);
        strcat(command, " -o ");
        strcat(command, objectPath);
        system(command);

        strcpy(command, "ld ");
        for (size_t i = 0; i < objects->size; i++) {
            strcat(command, list_get(objects, i));
            strcat(command, " ");
        }
        strcat(command, "-e _start -syslibroot $(xcrun -sdk macosx --show-sdk-path) -lSystem -o ");
        strcat(command, outputPath);
        system(command);
    }

    if (arch.kind == ARCH_X86_64) {
        char command[512];
        strcpy(command, "nasm -f macho64 ");
        strcat(command, assemblyPath);
        strcat(command, " -o ");
        strcat(command, objectPath);
        system(command);

        strcpy(command, "arch -x86_64 ld ");
        for (size_t i = 0; i < objects->size; i++) {
            strcat(command, list_get(objects, i));
            strcat(command, " ");
        }
        strcat(command, "-e _start -syslibroot $(xcrun -sdk macosx --show-sdk-path) -lSystem -o ");
        strcat(command, outputPath);
        system(command);
    }

    remove(objectPath);
    return EXIT_SUCCESS;
}
