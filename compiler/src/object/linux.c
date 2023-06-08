#include "object/linux.h"

#include <stdlib.h>
#include <string.h>

#include "object/object.h"
#include "utils/utils.h"

void object_linx_out(FILE *f, Program *program) {
    size_t origin = 0x0000000000400000;
    size_t alignment = 0x1000;

    // Create program header
    ELF64Program program_header[] = {
        // .text
        {
            .p_type = PT_LOAD,
            .p_flags = PF_R | PF_X,
            .p_offset = alignment,
            .p_vaddr = origin + alignment,
            .p_paddr = origin + alignment,
            .p_filesz = alignment,
            .p_memsz = alignment,
            .p_align = alignment,
        },
        // .data
        {
            .p_type = PT_LOAD,
            .p_flags = PF_R | PF_W,
            .p_offset = alignment + alignment,
            .p_vaddr = origin + alignment + alignment,
            .p_paddr = origin + alignment + alignment,
            .p_filesz = alignment,
            .p_memsz = alignment,
            .p_align = alignment,
        },
    };

    // Create section header
    char section_names[] = "\0.text\0.data\0.shstrtab\0";
    ELF64Section section_header[] = {
        {0},
        {
            .sh_name = 1,
            .sh_type = SHT_PROGBITS,
            .sh_flags = SHF_ALLOC | SHF_EXECINSTR,
            .sh_addr = origin + alignment,
            .sh_offset = alignment,
            .sh_size = program->text_section->filled,
            .sh_addralign = alignment,
        },
        {
            .sh_name = 7,
            .sh_type = SHT_PROGBITS,
            .sh_flags = SHF_ALLOC | SHF_WRITE,
            .sh_addr = origin + alignment + alignment,
            .sh_offset = alignment + alignment,
            .sh_size = program->data_section->filled,
            .sh_addralign = alignment,
        },
        {
            .sh_name = 13,
            .sh_type = SHT_STRTAB,
            .sh_offset = alignment + alignment + alignment,
            .sh_size = sizeof(section_names),
            .sh_addralign = 1,
        },
    };

    // Create ELF header
    ELF64Header header = {
        .ei_mag = {0x7f, 'E', 'L', 'F'},
        .ei_class = ELF64_CLASS_64,
        .ei_data = ELF64_DATA_LITTLE_ENDIAN,
        .ei_version = 1,
        .e_type = ELF64_TYPE_EXECUTE,
        .e_machine = ELF64_MACHINE_X86_64,
        .e_version = 1,
        .e_entry = origin + alignment,
        .e_phoff = sizeof(ELF64Header),
        .e_shoff = sizeof(ELF64Header) + sizeof(program_header),
        .e_ehsize = sizeof(ELF64Header),
        .e_phentsize = sizeof(ELF64Program),
        .e_phnum = 2,
        .e_shentsize = sizeof(ELF64Section),
        .e_shnum = 4,
        .e_shstrndx = 3,
    };

    // Write ELF parts
    fwrite(&header, sizeof(header), 1, f);
    fwrite(program_header, sizeof(program_header), 1, f);
    fwrite(section_header, sizeof(section_header), 1, f);
    size_t padding = alignment - ftell(f);
    for (size_t i = 0; i < padding; i++) {
        uint8_t zero = 0;
        fwrite(&zero, 1, 1, f);
    }

    fwrite(program->text_section->data, program->text_section->size, 1, f);
    fwrite(program->data_section->data, program->data_section->size, 1, f);

    fwrite(section_names, sizeof(section_names), 1, f);
}
