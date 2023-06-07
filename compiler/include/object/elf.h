#ifndef ELF_H
#define ELF_H

#include <stdint.h>

#define ELF64_CLASS_64 2
#define ELF64_DATA_LITTLE_ENDIAN 1
#define ELF64_TYPE_EXECUTE 2
#define ELF64_MACHINE_X86_64 0x3e
#define ELF64_MACHINE_AARCH64 0xb7

#define PT_LOAD 1  // Loadable segment type
#define PF_X 1     // Executable segment flag
#define PF_W 2     // Writable segment flag
#define PF_R 4     // Readable segment flag

#define SHT_NULL 0       // Null section type
#define SHT_PROGBITS 1   // Program-specific section type
#define SHT_STRTAB 3     // String table section type
#define SHF_WRITE 1      // Writable section flag
#define SHF_ALLOC 2      // Allocated in memory section flag
#define SHF_EXECINSTR 4  // Executable section flag

typedef struct {
    struct {
        uint8_t ei_mag[4];     // File identification bytes
        uint8_t ei_class;      // File class
        uint8_t ei_data;       // Data encoding
        uint8_t ei_version;    // File version
        uint8_t ei_osabi;      // Operating system/ABI identification
        uint8_t ei_osversion;  // ABI version
        uint8_t ei_pad[7];     // Padding bytes
    };
    uint16_t e_type;       // Object file type
    uint16_t e_machine;    // Architecture type
    uint32_t e_version;    // Version of the object file format
    uint64_t e_entry;      // Entry point virtual address
    uint64_t e_phoff;      // Program header table's file offset
    uint64_t e_shoff;      // Section header table's file offset
    uint32_t e_flags;      // Processor-specific flags
    uint16_t e_ehsize;     // Size of ELF header
    uint16_t e_phentsize;  // Size of each program header table entry
    uint16_t e_phnum;      // Number of entries in the program header table
    uint16_t e_shentsize;  // Size of each section header table entry
    uint16_t e_shnum;      // Number of entries in the section header table
    uint16_t e_shstrndx;   // Index of the section header table entry for the section name string table
} ELF64Header;

typedef struct {
    uint32_t p_type;    // Segment type
    uint32_t p_flags;   // Segment flags
    uint64_t p_offset;  // Segment offset in the file
    uint64_t p_vaddr;   // Virtual address of the segment in memory
    uint64_t p_paddr;   // Physical address of the segment (not used in most systems)
    uint64_t p_filesz;  // Size of segment in the file
    uint64_t p_memsz;   // Size of segment in memory (may be larger than p_filesz)
    uint64_t p_align;   // Alignment of segment in memory and file
} ELF64Program;

typedef struct {
    uint32_t sh_name;       // Section name (index into the section name string table)
    uint32_t sh_type;       // Section type
    uint64_t sh_flags;      // Section flags
    uint64_t sh_addr;       // Virtual address of the section in memory
    uint64_t sh_offset;     // Offset of the section in the ELF file
    uint64_t sh_size;       // Size of the section
    uint32_t sh_link;       // Link to other section (e.g., string table)
    uint32_t sh_info;       // Miscellaneous information (e.g., symbol table index)
    uint64_t sh_addralign;  // Required alignment of the section
    uint64_t sh_entsize;    // Size of each entry in the section (if applicable)
} ELF64Section;

#endif
