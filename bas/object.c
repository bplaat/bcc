// A experiment to generate a PE executable with some x86 instructions
// gcc object.c -o object.exe && ./object && objdump -S test.exe -Mintel && ./test
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

// Util
size_t align(size_t size, size_t alignment) { return (size + alignment - 1) / alignment * alignment; }

typedef struct List {
    void **items;
    size_t capacity;
    size_t size;
} List;

List *list_new(size_t capacity);

#define list_get(list, index) ((list)->items[index])

void list_add(List *list, void *item);

List *list_new(size_t capacity) {
    List *list = malloc(sizeof(List));
    list->items = malloc(sizeof(void *) * capacity);
    list->capacity = capacity;
    list->size = 0;
    return list;
}

void list_add(List *list, void *item) {
    if (list->size == list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, sizeof(void *) * list->capacity);
    }
    list->items[list->size++] = item;
}

// Platform
typedef enum PlatformKind {
    PLATFORM_WINDOWS
} PlatformKind;

typedef enum ArchKind {
    ARCH_X86,
    ARCH_X86_64
} ArchKind;

// Section
typedef enum SectionKind {
    SECTION_TEXT,
    SECTION_DATA
} SectionKind;

typedef struct Section {
    SectionKind kind;
    uint8_t *data;
    size_t size;
} Section;

Section *section_new(SectionKind kind, uint8_t *data, size_t size) {
    Section *section = malloc(sizeof(Section));
    section->kind = kind;
    section->data = data;
    section->size = size;
    return section;
}

// Object
typedef enum ObjectKind {
    OBJECT_EXECUTABLE
} ObjectKind;

typedef struct Object {
    PlatformKind platform;
    ArchKind arch;
    ObjectKind kind;
    List *sections;
} Object;

typedef struct pe_header {
    uint32_t signature;
    uint16_t machine;
    uint16_t numberOfSections;
    uint32_t timeDateStamp;
    uint32_t pointerToSytemTable;
    uint32_t numberOfSymbols;
    uint16_t sizeofOptionalHeader;
    uint16_t characteristics;
} pe_header;

typedef struct pe_optional_header32 {
    uint16_t magic;
    uint8_t majorLinkerVersion;
    uint8_t minorLinkerVersion;
    uint32_t sizeOfCode;
    uint32_t sizeOfInitializedData;
    uint32_t sizeOfUninitializedData;
    uint32_t addressOfEntryPoint;
    uint32_t baseOfCode;
    uint32_t baseOfData;
    uint32_t imageBase;
    uint32_t sectionAlignment;
    uint32_t fileAlignment;
    uint16_t majorOperatingSystemVersion;
    uint16_t minorOperatingSystemVersion;
    uint16_t majorImageVersion;
    uint16_t minorImageVersion;
    uint16_t majorSubsystemVersion;
    uint16_t minorSubsystemVersion;
    uint32_t win32VersionValue;
    uint32_t sizeOfImage;
    uint32_t sizeOfHeaders;
    uint32_t checkSum;
    uint16_t subsystem;
    uint16_t dllCharacteristics;
    uint32_t sizeOfStackReserve;
    uint32_t sizeOfStackCommit;
    uint32_t sizeOfHeapReserve;
    uint32_t sizeOfHeapCommit;
    uint32_t loaderFlags;
    uint32_t numberOfRvaAndSizes;
} pe_optional_header32;

typedef struct pe_optional_header64 {
    uint16_t magic;
    uint8_t majorLinkerVersion;
    uint8_t minorLinkerVersion;
    uint32_t sizeOfCode;
    uint32_t sizeOfInitializedData;
    uint32_t sizeOfUninitializedData;
    uint32_t addressOfEntryPoint;
    uint32_t baseOfCode;
    uint64_t imageBase;
    uint32_t sectionAlignment;
    uint32_t fileAlignment;
    uint16_t majorOperatingSystemVersion;
    uint16_t minorOperatingSystemVersion;
    uint16_t majorImageVersion;
    uint16_t minorImageVersion;
    uint16_t majorSubsystemVersion;
    uint16_t minorSubsystemVersion;
    uint32_t win32VersionValue;
    uint32_t sizeOfImage;
    uint32_t sizeOfHeaders;
    uint32_t checkSum;
    uint16_t subsystem;
    uint16_t dllCharacteristics;
    uint64_t sizeOfStackReserve;
    uint64_t sizeOfStackCommit;
    uint64_t sizeOfHeapReserve;
    uint64_t sizeOfHeapCommit;
    uint32_t loaderFlags;
    uint32_t numberOfRvaAndSizes;
} pe_optional_header64;

typedef struct pe_section {
    char name[8];
    uint32_t virtualSize;
    uint32_t virtualAddress;
    uint32_t sizeOfRawData;
    uint32_t pointerToRawData;
    uint32_t pointerToRelocations;
    uint32_t pointerToLinenumbers;
    uint16_t numberOfRelocations;
    uint16_t numberOfLinenumbers;
    uint32_t characteristics;
} pe_section;

Object *object_new(PlatformKind platform, ArchKind arch, ObjectKind kind) {
    Object *object = malloc(sizeof(Object));
    object->platform = platform;
    object->arch = arch;
    object->kind = kind;
    object->sections = list_new(4);
    return object;
}

uint8_t msdos_header[] = {
    // MS-DOS Header
    0x4D, 0x5A, 0x90, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
    0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,

    // MS-DOS Stub
    0x0E, 0x1F, 0xBA, 0x0E, 0x00, 0xB4, 0x09, 0xCD, 0x21, 0xB8, 0x01, 0x4C, 0xCD, 0x21, 0x54, 0x68,
    0x69, 0x73, 0x20, 0x70, 0x72, 0x6F, 0x67, 0x72, 0x61, 0x6D, 0x20, 0x63, 0x61, 0x6E, 0x6E, 0x6F,
    0x74, 0x20, 0x62, 0x65, 0x20, 0x72, 0x75, 0x6E, 0x20, 0x69, 0x6E, 0x20, 0x44, 0x4F, 0x53, 0x20,
    0x6D, 0x6F, 0x64, 0x65, 0x2E, 0x0D, 0x0D, 0x0A, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void object_write(Object *object, char *path) {
    FILE *f = fopen(path, "wb");

    if (object->platform == PLATFORM_WINDOWS) {
        uint64_t origin = 0x400000;
        uint32_t sectionAlign = 0x200; // 0x1000; // TODO
        uint32_t fileAlign = 0x200;

        // MSDOS Header
        fwrite(msdos_header, sizeof(msdos_header), 1, f);

        // PE Header
        uint32_t machine, sizeofOptionalHeader;
        if (object->arch == ARCH_X86) {
            machine = 0x014C;
            sizeofOptionalHeader = sizeof(pe_optional_header32) + 8 * 16;
        }
        if (object->arch == ARCH_X86_64) {
            machine = 0x8664;
            sizeofOptionalHeader = sizeof(pe_optional_header64) + 8 * 16;
        }
        pe_header header = {
            .signature = 0x4550,
            .machine = machine,
            .numberOfSections = object->sections->size,
            .timeDateStamp = time(NULL),
            .pointerToSytemTable = 0,
            .numberOfSymbols = 0,
            .sizeofOptionalHeader = sizeofOptionalHeader,
            .characteristics = 0x30f
        };
        fwrite(&header, sizeof(header), 1, f);

        // Optional Header
        size_t sizeOfCode = 0;
        size_t sizeOfInitializedData = 0;
        size_t baseOfCode = 0;
        size_t baseOfData = 0;

        size_t virtualOffset = sectionAlign;
        size_t fileOffset = fileAlign;
        for (size_t i = 0; i < object->sections->size; i++) {
            Section *section = list_get(object->sections, i);
            if (section->kind == SECTION_TEXT) {
                sizeOfCode = align(section->size, fileAlign);
                baseOfCode = virtualOffset;
            }
            if (section->kind == SECTION_DATA) {
                sizeOfInitializedData = align(section->size, fileAlign);
                baseOfData = virtualOffset;
            }
            virtualOffset += align(section->size, sectionAlign);
            fileOffset += align(section->size, fileAlign);
        }
        size_t sizeOfImage = fileOffset;

        if (object->arch == ARCH_X86) {
            pe_optional_header32 optional_header = {
                .magic = 0x010b,
                .majorLinkerVersion = 0,
                .minorLinkerVersion = 0,
                .sizeOfCode = sizeOfCode,
                .sizeOfInitializedData = sizeOfInitializedData,
                .sizeOfUninitializedData = 0,
                .addressOfEntryPoint = baseOfCode,
                .baseOfCode = baseOfCode,
                .baseOfData = baseOfData,
                .imageBase = origin,
                .sectionAlignment = sectionAlign,
                .fileAlignment = fileAlign,
                .majorOperatingSystemVersion = 4,
                .minorOperatingSystemVersion = 0,
                .majorImageVersion = 0,
                .minorImageVersion = 0,
                .majorSubsystemVersion = 4,
                .minorSubsystemVersion = 0,
                .win32VersionValue = 0,
                .sizeOfImage = sizeOfImage,
                .sizeOfHeaders = fileAlign,
                .checkSum = 0, // TODO
                .subsystem = 3,
                .dllCharacteristics = 0,
                .sizeOfStackReserve = 0x100000,
                .sizeOfStackCommit = 0x1000,
                .sizeOfHeapReserve = 0x100000,
                .sizeOfHeapCommit = 0x1000,
                .loaderFlags = 0,
                .numberOfRvaAndSizes = 16
            };
            fwrite(&optional_header, sizeof(optional_header), 1, f);
        }

        if (object->arch == ARCH_X86_64) {
            pe_optional_header64 optional_header = {
                .magic = 0x020B,
                .majorLinkerVersion = 0,
                .minorLinkerVersion = 0,
                .sizeOfCode = sizeOfCode,
                .sizeOfInitializedData = sizeOfInitializedData,
                .sizeOfUninitializedData = 0,
                .addressOfEntryPoint = baseOfCode,
                .baseOfCode = baseOfCode,
                .imageBase = origin,
                .sectionAlignment = sectionAlign,
                .fileAlignment = fileAlign,
                .majorOperatingSystemVersion = 4,
                .minorOperatingSystemVersion = 0,
                .majorImageVersion = 0,
                .minorImageVersion = 0,
                .majorSubsystemVersion = 4,
                .minorSubsystemVersion = 0,
                .win32VersionValue = 0,
                .sizeOfImage = sizeOfImage,
                .sizeOfHeaders = fileAlign,
                .checkSum = 0, // TODO
                .subsystem = 3,
                .dllCharacteristics = 0,
                .sizeOfStackReserve = 0x100000,
                .sizeOfStackCommit = 0x1000,
                .sizeOfHeapReserve = 0x100000,
                .sizeOfHeapCommit = 0x1000,
                .loaderFlags = 0,
                .numberOfRvaAndSizes = 16
            };
            fwrite(&optional_header, sizeof(optional_header), 1, f);
        }

        uint32_t dataDirective[2] = {0};
        for (int32_t i = 0; i < 16; i++) {
            fwrite(&dataDirective, sizeof(dataDirective), 1, f);
        }

        virtualOffset = sectionAlign;
        fileOffset = fileAlign;
        for (size_t i = 0; i < object->sections->size; i++) {
            Section *section = list_get(object->sections, i);
            pe_section section_header = {
                .virtualSize = section->size,
                .virtualAddress = virtualOffset,
                .sizeOfRawData = align(section->size, fileAlign),
                .pointerToRawData = fileOffset,
                .pointerToRelocations = 0,
                .pointerToLinenumbers = 0,
                .numberOfRelocations = 0,
                .numberOfLinenumbers = 0
            };
            if (section->kind == SECTION_TEXT) {
                strcpy(section_header.name, ".text");
                section_header.characteristics = 0x60000020;
            }
            if (section->kind == SECTION_DATA) {
                strcpy(section_header.name, ".data");
                section_header.characteristics = 0xC0000040;
            }
            fwrite(&section_header, sizeof(section_header), 1, f);
            virtualOffset += align(section->size, sectionAlign);
            fileOffset += align(section->size, fileAlign);
        }

        size_t alignment = 0x200 - ftell(f);
        for (size_t i = 0; i < alignment; i++) {
            uint8_t zero = 0;
            fwrite(&zero, sizeof(zero), 1, f);
        }

        // Sections data
        for (size_t i = 0; i < object->sections->size; i++) {
            Section *section = list_get(object->sections, i);
            fwrite(section->data, section->size, 1, f);

            size_t alignment = align(section->size, fileAlign) - section->size;
            for (size_t i = 0; i < alignment; i++) {
                uint8_t zero = 0;
                fwrite(&zero, sizeof(zero), 1, f);
            }
        }
    }

    fclose(f);
}

// ################################################################

typedef enum InstKind {
    INST_NOP,
    INST_MOV,
    INST_ADD,
    INST_SUB,
    INST_PUSH,
    INST_POP,
    INST_RET,
    INST_SYSCALL
} InstKind;

typedef enum OprandKind {
    OPRAND_IMM,
    OPRAND_REG,
    OPRAND_ADDR
} OprandKind;

typedef struct Oprand {
    OprandKind kind;
    int32_t size;
    union {
        int64_t imm;
        struct {
            int32_t reg;
            int32_t disp;
        };
    };
} Oprand;

void IMM32(uint8_t **end, int32_t imm) {
    int32_t *c = (int32_t *)*end;
    *c++ = imm;
    *end = (uint8_t *)c;
}

void IMM64(uint8_t **end, int64_t imm) {
    int64_t *c = (int64_t *)*end;
    *c++ = imm;
    *end = (uint8_t *)c;
}

void inst(uint8_t **end, InstKind kind, Oprand *dest, Oprand *src) {
    uint8_t *c = *end;

    if (kind == INST_NOP) {
        *c++ = 0x90;
    }

    if (kind >= INST_MOV && kind <= INST_SUB) {
        int32_t opcode, opcode2;
        if (kind == INST_MOV) opcode = 0x89, opcode2 = 0b000;
        if (kind == INST_ADD) opcode = 0x01, opcode2 = 0b000;
        if (kind == INST_SUB) opcode = 0x29, opcode2 = 0b101;
        if (dest->size == 64) *c++ = 0x48;
        if (dest->kind == OPRAND_REG) {
            if (src->kind == OPRAND_REG) {
                *c++ = opcode;
                *c++ = (0b11 << 6) | (src->reg << 3) | dest->reg;
            }
            if (src->kind == OPRAND_ADDR) {
                *c++ = opcode | 0b10;
                if (dest->disp <= 0xff) {
                    *c++ = (0b01 << 6) | (src->reg << 3) | dest->reg;
                    *c++ = dest->disp;
                } else {
                    *c++ = (0b10 << 6) | (src->reg << 3) | dest->reg;
                    IMM32(&c, dest->disp);
                }
            }
            if (src->kind == OPRAND_IMM) {
                if (kind == INST_MOV) {
                    *c++ = 0xb8 + dest->reg;
                    if (dest->size == 64) {
                        IMM64(&c, src->imm);
                    } else {
                        IMM32(&c, src->imm);
                    }
                } else {
                    *c++ = src->imm <= 0xff ? 0x83 : 0x81;
                    *c++ = (0b11 << 6) | (opcode2 << 3) | dest->reg;
                    if (src->imm <= 0xff) {
                        *c++ = src->imm;
                    } else {
                        IMM32(&c, src->imm);
                    }
                }
            }
        }
        if (dest->kind == OPRAND_ADDR) {
            if (src->kind == OPRAND_REG) {
                *c++ = opcode;
                if (dest->disp <= 0xff) {
                    *c++ = (0b01 << 6) | (src->reg << 3) | dest->reg;
                    *c++ = dest->disp;
                } else {
                    *c++ = (0b10 << 6) | (src->reg << 3) | dest->reg;
                    IMM32(&c, dest->disp);
                }
            }
            if (src->kind == OPRAND_IMM) {
                *c++ = src->imm <= 0xff ? (kind == INST_MOV ? 0xc6 : 0x83) : (kind == INST_MOV ? 0xc7 : 0x81);
                if (dest->disp <= 0xff) {
                    *c++ = (0b01 << 6) | (opcode2 << 3) | dest->reg;
                    *c++ = dest->disp;
                } else {
                    *c++ = (0b10 << 6) | (opcode2 << 3) | dest->reg;
                    IMM32(&c, dest->disp);
                }
                if (src->imm <= 0xff) {
                    *c++ = src->imm;
                } else {
                    IMM32(&c, src->imm);
                }
            }
        }
    }

    if (kind == INST_PUSH) {
        if (src->kind == OPRAND_REG) {
            *c++ = 0x50 + src->reg;
        }
    }

    if (kind == INST_POP) {
        if (dest->kind == OPRAND_REG) {
            *c++ = 0x58 + dest->reg;
        }
    }

    if (kind == INST_RET) {
        *c++ = 0xc3;
    }

    if (kind == INST_SYSCALL) {
        *c++ = 0x0f;
        *c++ = 0x05;
    }

    *end = c;
}


#define EAX .size = 32, .reg = 0
#define ECX .size = 32, .reg = 1
#define EDX .size = 32, .reg = 2
#define EBX .size = 32, .reg = 3
#define ESP .size = 32, .reg = 4
#define EBP .size = 32, .reg = 5
#define ESI .size = 32, .reg = 6
#define EDI .size = 32, .reg = 7
#define RAX .size = 64, .reg = 0
#define RCX .size = 64, .reg = 1
#define RDX .size = 64, .reg = 2
#define RBX .size = 64, .reg = 3
#define RSP .size = 64, .reg = 4
#define RBP .size = 64, .reg = 5
#define RSI .size = 64, .reg = 6
#define RDI .size = 64, .reg = 7

int main(int argc, char **argv) {

    Object *object = object_new(PLATFORM_WINDOWS, ARCH_X86_64, OBJECT_EXECUTABLE);

    uint8_t text[0x200];
    uint8_t *c = text;


    // inst(&c, INST_MOV, &(Oprand){ OPRAND_REG, EAX }, &(Oprand){ OPRAND_IMM, 32, .imm = -1 });

    // inst(&c, INST_NOP, NULL, NULL);

    // inst(&c, INST_ADD, &(Oprand){ OPRAND_ADDR, RDX, .disp = 0xaa }, &(Oprand){ OPRAND_IMM, 32, .imm = 0xaabb });

    // inst(&c, INST_ADD, &(Oprand){ OPRAND_REG, RDX }, &(Oprand){ OPRAND_REG, RAX });
    // inst(&c, INST_MOV, &(Oprand){ OPRAND_REG, EDX }, &(Oprand){ OPRAND_IMM, 32, .imm = 0xaaaa });
    // inst(&c, INST_ADD, &(Oprand){ OPRAND_REG, EDX }, &(Oprand){ OPRAND_IMM, 32, .imm = 0x1234 });
    // inst(&c, INST_SUB, &(Oprand){ OPRAND_REG, EDX }, &(Oprand){ OPRAND_IMM, 32, .imm = 8 });

    // inst(&c, INST_NOP, NULL, NULL);

    // inst(&c, INST_PUSH, NULL, &(Oprand){ OPRAND_REG, EDX });
    // inst(&c, INST_POP, &(Oprand){ OPRAND_REG, EDX }, NULL);


    // inst(&c, INST_SUB, &(Oprand){ OPRAND_REG, EDX }, &(Oprand){ OPRAND_REG, EAX });
    // inst(&c, INST_SUB, &(Oprand){ OPRAND_REG, EDX }, &(Oprand){ OPRAND_IMM, 32, .imm = 0xaaaa });

    // inst(&c, INST_SUB, &(Oprand){ OPRAND_REG, EAX }, &(Oprand){ OPRAND_ADDR, EBP, .disp = 0xaa });

    Oprand rdx = { OPRAND_REG, 64, 3 };

    inst(&c, INST_MOV, &(Oprand){ OPRAND_ADDR32, rdx, .disp = 8 }, &(Oprand){ OPRAND_IMM, .imm = 0xaa });

    inst(&c, INST_RET, NULL, NULL);

    inst(&c, INST_SYSCALL, NULL, NULL);

    list_add(object->sections, section_new(SECTION_TEXT, text, c - text));

    object_write(object, "test.exe");

    return EXIT_SUCCESS;
}
