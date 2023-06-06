#ifndef OBJECT_H
#define OBJECT_H

#include <stdbool.h>
#include <stdio.h>

#ifdef _WIN32

#define MEM_COMMIT 0x00001000
#define MEM_RESERVE 0x00002000
#define SECTION_READWRITE 0x04
#define SECTION_EXECUTE_READ 0x20
#define MEM_RELEASE 0x00008000

extern void *VirtualAlloc(void *lpAddress, size_t dwSize, uint32_t flAllocationType, uint32_t flProtect);
extern bool VirtualProtect(void *lpAddress, size_t dwSize, uint32_t flNewProtect, uint32_t *lpflOldProtect);
extern bool VirtualFree(void *lpAddress, size_t dwSize, uint32_t dwFreeType);

#else

#include <sys/mman.h>

#endif

// Section
typedef struct Section {
    void *data;
    size_t size;
    size_t filled;
} Section;

Section *section_new(size_t size);

bool section_make_executable(Section *section);

void section_dump(FILE *f, Section *section);

void section_free(Section *section);

#endif
