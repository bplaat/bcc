#include "codegen.h"

void codegen(Arch arch, void *code, char *text, Node *node, void **main) {
    Codegen codegen = {
        .text = text,
        .code = code,
        .code_byte_ptr = (uint8_t *)code,
        .code_word_ptr = (uint32_t *)code,
    };
    if (arch == ARCH_X86_64) codegen_node_x86_64(&codegen, node);
    if (arch == ARCH_ARM64) codegen_node_arm64(&codegen, node);
    *main = codegen.main;
}
