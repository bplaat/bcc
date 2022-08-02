#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "lexer.h"
#include "parser.h"

int main(int argc, char **argv) {
    // Architectures
    Arch arm64Arch = {.kind = ARCH_ARM64,
                      .wordSize = 4,
                      .pointerSize = 8,
                      .stackAlign = 16,
                      .regs32 = (char *[]){"w0", "w1", "w2", "w3", "w4", "w5", "w6", "w7"},
                      .regs64 = (char *[]){"x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7"},
                      .regsSize = 8,
                      .argRegs = (int32_t[]){0, 1, 2, 3, 4, 5, 6, 7},
                      .argRegsSize = 8,
                      .returnReg = 0};

    Arch x86_64Arch = {.kind = ARCH_X86_64,
                       .wordSize = 4,
                       .pointerSize = 8,
                       .stackAlign = 8,
                       .regs8 = (char *[]){"al", "dil", "sil", "dl", "cl", "r8l", "r9l", "r10l", "r11l"},
                       .regs16 = (char *[]){"ax", "di", "si", "dx", "cx", "r8w", "r9w", "r10w", "r11w"},
                       .regs32 = (char *[]){"eax", "edi", "esi", "edx", "ecx", "r8d", "r9d", "r10d", "r11d"},
                       .regs64 = (char *[]){"rax", "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11"},
                       .regsSize = 9,
                       .argRegs = (int32_t[]){1, 2, 3, 4, 5, 6},
                       .argRegsSize = 6,
                       .returnReg = 0};

    // Parse args
    Arch *arch = &arm64Arch;
    char *text = NULL;
    List *objects = list_new(8);
    char *outputPath = NULL;
    bool dumpNode = false;
    for (int32_t i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-o")) {
            i++;
            outputPath = argv[i];
            continue;
        }

        if (!strcmp(argv[i], "-d")) {
            dumpNode = true;
            continue;
        }

        if (!strcmp(argv[i], "-arch")) {
            i++;
            if (!strcmp(argv[i], "arm64")) {
                arch = &arm64Arch;
            }
            if (!strcmp(argv[i], "x86_64")) {
                arch = &x86_64Arch;
            }
            continue;
        }

        if (text == NULL) {
            text = argv[i];
        } else {
            list_add(objects, argv[i]);
        }
    }
    if (argc == 1 || text == NULL) {
        printf("Bassie C Compiler\n");
        return EXIT_SUCCESS;
    }

    // Compile
    List *tokens = lexer(text);
    Node *node = parser(arch, text, tokens);
    if (dumpNode) {
        printf("%s", node_to_string(node));
        return EXIT_SUCCESS;
    }

    // Codegen
    FILE *f = stdout;
    char assemblyPath[512];
    if (outputPath != NULL) {
        sprintf(assemblyPath, "%s.s", outputPath);
        f = fopen(assemblyPath, "w");
    }
    if (arch->kind == ARCH_ARM64) {
        fprintf(f, ".macro push reg\n");
        fprintf(f, "    str \\reg, [sp, -%d]!\n", arch->stackAlign);
        fprintf(f, ".endm\n");
        fprintf(f, ".macro pop reg\n");
        fprintf(f, "    ldr \\reg, [sp], %d\n", arch->stackAlign);
        fprintf(f, ".endm\n");
        fprintf(f, ".balign 4\n\n");
    }
    if (arch->kind == ARCH_X86_64) {
        fprintf(f, ".intel_syntax noprefix\n\n");
    }

    fprintf(f, ".global _start\n");
    fprintf(f, "_start:\n");
    if (arch->kind == ARCH_ARM64) {
        fprintf(f, "    bl _main\n");
        fprintf(f, "    mov x16, 1\n");
        fprintf(f, "    svc 0x80\n");
    }
    if (arch->kind == ARCH_X86_64) {
        fprintf(f, "    call _main\n");
        fprintf(f, "    mov rdi, rax\n");
        fprintf(f, "    mov eax, 0x2000001\n");
        fprintf(f, "    syscall\n");
    }

    codegen(f, arch, node);
    fclose(f);
    if (outputPath == NULL) {
        return EXIT_SUCCESS;
    }

    // Assemble assembly file and link executable
    char objectPath[512];
    sprintf(objectPath, "%s.o", outputPath);
    list_add(objects, objectPath);

    if (arch->kind == ARCH_ARM64) {
        char command[512];
        sprintf(command, "as %s -o %s", assemblyPath, objectPath);
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

    if (arch->kind == ARCH_X86_64) {
        char command[512];
        sprintf(command, "arch -x86_64 as %s -o %s", assemblyPath, objectPath);
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
