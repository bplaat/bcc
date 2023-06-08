#include "object/object.h"

#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/stat.h>
#endif

#include "object/elf.h"
#include "utils.h"

// Object
void object_out(ObjectKind kind, char *path) {
    FILE *f = fopen(path, "wb");

    if (kind == OBJECT_ELF) {
        size_t origin = 0x0000000000400000;
        size_t alignment = 0x1000;

        uint8_t section_text[0x1000] = { 0xBF, 0x45, 0x00, 0x00, 0x00, 0xB8, 0x3C, 0x00, 0x00, 0x00, 0x0F, 0x05 };
        size_t section_text_size = 12;

        uint8_t section_data[0x1000];
        size_t section_data_size = 0;
        section_data[section_data_size++] = 'H';
        section_data[section_data_size++] = 'o';
        section_data[section_data_size++] = 'i';
        section_data[section_data_size++] = 0x00;

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
                .sh_size = section_text_size,
                .sh_addralign = alignment,
            },
            {
                .sh_name = 7,
                .sh_type = SHT_PROGBITS,
                .sh_flags = SHF_ALLOC | SHF_WRITE,
                .sh_addr = origin + alignment + alignment,
                .sh_offset = alignment + alignment,
                .sh_size = section_data_size,
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

        // Write parts
        fwrite(&header, sizeof(header), 1, f);
        fwrite(program_header, sizeof(program_header), 1, f);
        fwrite(section_header, sizeof(section_header), 1, f);

        size_t padding = alignment - ftell(f);
        for (size_t i = 0; i < padding; i++) {
            uint8_t zero = 0;
            fwrite(&zero, 1, 1, f);
        }

        fwrite(section_text, sizeof(section_text), 1, f);
        fwrite(section_data, sizeof(section_data), 1, f);
        fwrite(section_names, sizeof(section_names), 1, f);
    }

    fclose(f);

// Give file executable rights
#ifndef _WIN32
    chmod(path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif
}

// Section
Section *section_new(size_t size) {
    Section *section = calloc(1, sizeof(Section));
    section->size = size;
#ifdef _WIN32
    section->data = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, SECTION_READWRITE);
#else
    section->data = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
    return section;
}

bool section_make_executable(Section *section) {
#ifdef _WIN32
    uint32_t oldProtect;
    return VirtualProtect(section->data, section->size, SECTION_EXECUTE_READ, &oldProtect) != 0;
#else
    return mprotect(section->data, section->size, PROT_READ | PROT_EXEC) != -1;
#endif
}

void section_dump(FILE *f, Section *section) {
    uint8_t *bytes = section->data;
    size_t i = 0;
    for (size_t y = 0; y < align(section->filled, 16); y += 16) {
        for (int32_t x = 0; x < 16; x++) {
            if (i < section->filled) {
                fprintf(f, "%02x ", bytes[i++]);
            }
        }
        fprintf(f, "\n");
    }
}

void section_free(Section *section) {
#ifdef _WIN32
    VirtualFree(section->data, 0, MEM_RELEASE);
#else
    munmap(section->data, section->size);
#endif
    free(section);
}
