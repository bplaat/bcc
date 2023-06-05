#include "codegen.h"

void codegen(Program *program) {
    Codegen codegen = {
        .program = program,
        .code_byte_ptr = (uint8_t *)program->section_text->data,
        .code_word_ptr = (uint32_t *)program->section_text->data,
    };

    // x86_64
    if (program->arch == ARCH_X86_64) {
        for (size_t i = 0; i < program->functions.size; i++) {
            Function *function = program->functions.items[i];
            codegen_func_x86_64(&codegen, function);
        }
    }

    // arm64
    if (program->arch == ARCH_ARM64) {
        for (size_t i = 0; i < program->functions.size; i++) {
            Function *function = program->functions.items[i];
            codegen_func_arm64(&codegen, function);
        }
    }
}
