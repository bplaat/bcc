#ifndef PAGE_H
#define PAGE_H

#include <stdbool.h>

#ifdef _WIN32

#define MEM_COMMIT 0x00001000
#define MEM_RESERVE 0x00002000
#define PAGE_READWRITE 0x04
#define PAGE_EXECUTE_READ 0x20
#define MEM_RELEASE 0x00008000

extern void *VirtualAlloc(void *lpAddress, size_t dwSize, uint32_t flAllocationType, uint32_t flProtect);
extern bool VirtualProtect(void *lpAddress, size_t dwSize, uint32_t flNewProtect, uint32_t *lpflOldProtect);
extern bool VirtualFree(void *lpAddress, size_t dwSize, uint32_t dwFreeType);

#else

#include <sys/mman.h>

#endif

typedef struct Page {
    void *data;
    size_t size;
} Page;

Page *page_new(size_t size);
bool page_make_executable(Page *page);
void page_free(Page *page);

#endif
