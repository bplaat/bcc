#include "codegen.h"

#include <string.h>

void codegen(Program *program) {
    Codegen codegen = {
        .program = program,
        .code_byte_ptr = (uint8_t *)program->text_section->data,
        .code_word_ptr = (uint32_t *)program->text_section->data,
    };

    // Fill globals in data section
    uint8_t *global_address = program->data_section->data;
    for (size_t i = 0; i < program->globals.size; i++) {
        Global *global = program->globals.items[i];
        global->address = global_address;
        memset(global->address, 0, global->type->size);
        global_address += global->type->size;
    }
    program->data_section->filled = global_address- (uint8_t *)program->data_section->data;

    // x86_64
    if (program->arch == ARCH_X86_64) {
        for (size_t i = 0; i < program->functions.size; i++) {
            Function *function = program->functions.items[i];
            codegen_func_x86_64(&codegen, function);
            program->text_section->filled = codegen.code_byte_ptr - (uint8_t *)program->text_section->data;
        }
    }

    // arm64
    if (program->arch == ARCH_ARM64) {
        for (size_t i = 0; i < program->functions.size; i++) {
            Function *function = program->functions.items[i];
            codegen_func_arm64(&codegen, function);
            program->text_section->filled = (uint8_t *)codegen.code_word_ptr - (uint8_t *)program->text_section->data;
        }
    }
}
