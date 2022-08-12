#include "object.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utils.h"

// Section
Section *section_new(SectionKind kind, char *name, uint8_t *data, size_t size) {
    Section *section = malloc(sizeof(Section));
    section->kind = kind;
    section->name = name;
    section->data = data;
    section->size = size;
    return section;
}

// Symbol
Symbol *symbol_new(char *name, Section *section, uint32_t value) {
    Symbol *symbol = malloc(sizeof(Symbol));
    symbol->name = name;
    symbol->section = section;
    symbol->value = value;
    return symbol;
}

// Object
Object *object_new(Platform *platform, Arch *arch, ObjectKind kind) {
    Object *object = malloc(sizeof(Object));
    object->platform = platform;
    object->arch = arch;
    object->kind = kind;
    object->sections = list_new(4);
    object->symbols = list_new(8);
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

    if (object->platform->kind == PLATFORM_WINDOWS) {
        uint64_t origin = 0x400000;
        uint32_t sectionAlign = 0x1000;
        uint32_t fileAlign = 0x200;

        // Calculate sizes
        size_t sizeOfCode = 0;
        size_t baseOfCode = 0;
        size_t sizeOfInitializedData = 0;
        size_t virtualOffset = sectionAlign;
        size_t fileOffset = fileAlign;
        for (size_t i = 0; i < object->sections->size; i++) {
            Section *section = list_get(object->sections, i);
            if (section->kind == SECTION_TEXT) {
                sizeOfCode = align(section->size, fileAlign);
                baseOfCode = virtualOffset;
            }
            if (section->kind == SECTION_DATA || section->kind == SECTION_READ_ONLY_DATA) {
                sizeOfInitializedData = align(section->size, fileAlign);
            }
            virtualOffset += align(section->size, sectionAlign);
            fileOffset += align(section->size, fileAlign);
        }
        size_t symbolsSize = align(object->symbols->size * sizeof(pe_symbol), fileAlign);
        fileOffset += symbolsSize;

        // MSDOS Header
        fwrite(msdos_header, sizeof(msdos_header), 1, f);

        // PE Header
        uint32_t machine, sizeofOptionalHeader;
        if (object->arch->kind == ARCH_X86_64) {
            machine = 0x8664;
            sizeofOptionalHeader = sizeof(pe_optional_header64) + 8 * 16;
        }
        pe_header header = {
            .signature = 0x4550,
            .machine = machine,
            .numberOfSections = object->sections->size,
            .timeDateStamp = time(NULL),
            .pointerToSytemTable = object->symbols->size > 0 ? fileOffset - symbolsSize : 0,
            .numberOfSymbols = object->symbols->size,
            .sizeofOptionalHeader = sizeofOptionalHeader,
            .characteristics = 0x0200 | 0x0020 | 0x0003
        };
        fwrite(&header, sizeof(header), 1, f);

        // Optional Header
        if (object->arch->kind == ARCH_X86_64) {
            pe_optional_header64 optional_header = {
                .magic = 0x020B,
                .majorLinkerVersion = 6,
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
                .sizeOfImage = virtualOffset,
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
            strcpy(section_header.name, section->name);
            if (section->kind == SECTION_TEXT) {
                section_header.characteristics = 0x60000020;
            }
            if (section->kind == SECTION_DATA) {
                section_header.characteristics = 0xC0000040;
            }
            if (section->kind == SECTION_READ_ONLY_DATA) {
                section_header.characteristics = 0x40000040;
            }
            fwrite(&section_header, sizeof(section_header), 1, f);
            virtualOffset += align(section->size, sectionAlign);
            fileOffset += align(section->size, fileAlign);
        }

        size_t alignment = fileAlign - ftell(f);
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

        // Symbols table
        if (object->symbols->size > 0) {
            for (size_t i = 0; i < object->symbols->size; i++) {
                Symbol *symbol = list_get(object->symbols, i);
                pe_symbol _symbol = {
                    .value = symbol->value,
                    .sectionNumber = 1,
                    .type = 0x2000,
                    .storageClass = 6,
                    .numberOfAuxSymbols = 0
                };
                strcpy(_symbol.name, symbol->name);
                fwrite(&_symbol, sizeof(_symbol), 1, f);
            }

            size_t alignment = symbolsSize - object->symbols->size * sizeof(pe_symbol);
            for (size_t i = 0; i < alignment; i++) {
                uint8_t zero = 0;
                fwrite(&zero, sizeof(zero), 1, f);
            }

        }
    }

    fclose(f);
}
