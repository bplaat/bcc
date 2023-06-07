#include "object.h"

#include <stdlib.h>

#include "utils.h"

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
