#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "lexer.h"
#include "parser.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        return EXIT_FAILURE;
    }

    Arch arch;
    arch.kind = ARCH_ARM64;
    arch.stackAlign = 16;

    if (argc >= 3 && !strcmp(argv[2], "--arch=x86_64")) {
        arch.kind = ARCH_X86_64;
        arch.stackAlign = 8;
    }

    List *tokens = lexer(argv[1]);
    Node *node = parser(argv[1], tokens, &arch);
    FILE *out = fopen("a.s", "w");

    if (arch.kind == ARCH_ARM64) {
        fprintf(out, "    .align 4\n");
        fprintf(out, "    .global _start\n");
    }
    if (arch.kind == ARCH_X86_64) {
        fprintf(out, "    global _start\n");
    }

    fprintf(out, "_start:\n");
    if (arch.kind == ARCH_ARM64) {
        fprintf(out, "    bl _main\n");
        fprintf(out, "    mov x16, #1\n");
        fprintf(out, "    svc #0x80\n\n");
    }
    if (arch.kind == ARCH_X86_64) {
        fprintf(out, "    call _main\n");
        fprintf(out, "    mov edi, eax\n");
        fprintf(out, "    mov eax, 0x2000001\n");
        fprintf(out, "    syscall\n\n");
    }

    codegen(out, node, &arch);
    fclose(out);

    if (arch.kind == ARCH_ARM64) {
        system("as a.s -o a.o");
        system("ld a.o b.o -e _start -syslibroot $(xcrun -sdk macosx --show-sdk-path) -lSystem -o a.out");
    }
    if (arch.kind == ARCH_X86_64) {
        system("nasm -f macho64 a.s -o a.o");
        system("arch -x86_64 ld a.o b.o -e _start -syslibroot $(xcrun -sdk macosx --show-sdk-path) -lSystem -o a.out");
    }
    remove("a.o");
    remove("b.o");

    node_print(stdout, node);
    return EXIT_SUCCESS;
}
