#pragma once

#include <stdint.h>
#include "list.h"

// Platform
typedef enum PlatformKind {
    PLATFORM_WINDOWS
} PlatformKind;

typedef struct Platform {
    PlatformKind kind;
} Platform;

// Arch
typedef enum ArchKind {
    ARCH_X86_64
} ArchKind;

typedef struct Arch {
    ArchKind kind;
} Arch;

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

Section *section_new(SectionKind kind, uint8_t *data, size_t size);

// Symbol
typedef struct Symbol {
    char *name;
    Section *section;
    uint32_t value;
} Symbol;

Symbol *symbol_new(char *name, Section *section, uint32_t value);

// Object
typedef enum ObjectKind {
    OBJECT_EXECUTABLE
} ObjectKind;

typedef struct Object {
    Platform *platform;
    Arch *arch;
    ObjectKind kind;
    List *sections;
    List *symbols;
} Object;

typedef struct __attribute__((packed)) pe_header {
    uint32_t signature;
    uint16_t machine;
    uint16_t numberOfSections;
    uint32_t timeDateStamp;
    uint32_t pointerToSytemTable;
    uint32_t numberOfSymbols;
    uint16_t sizeofOptionalHeader;
    uint16_t characteristics;
} pe_header;

typedef struct __attribute__((packed)) pe_optional_header64 {
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

typedef struct __attribute__((packed)) pe_section {
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

typedef struct __attribute__((packed)) pe_symbol {
    char name[8];
    uint32_t value;
    uint16_t sectionNumber;
    uint16_t type;
    uint8_t storageClass;
    uint8_t numberOfAuxSymbols;
} pe_symbol;

Object *object_new(Platform *platform, Arch *arch, ObjectKind kind);

void object_write(Object *object, char *path);
