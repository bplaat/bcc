#include "page.h"

#include <stdlib.h>

Page *page_new(size_t size) {
    Page *page = malloc(sizeof(Page));
    page->size = size;
#ifdef _WIN32
    page->data = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (page->data == NULL) {
        return NULL;
    }
#else
    page->data = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (page->data == (void *)-1) {
        return NULL;
    }
#endif
    return page;
}

bool page_make_executable(Page *page) {
#ifdef _WIN32
    uint32_t oldProtect;
    return VirtualProtect(page->data, page->size, PAGE_EXECUTE_READ, &oldProtect) != 0;
#else
    return mprotect(page->data, page->size, PROT_READ | PROT_EXEC) != -1;
#endif
}

void page_free(Page *page) {
#ifdef _WIN32
    VirtualFree(page->data, 0, MEM_RELEASE);
#else
    munmap(page->data, page->size);
#endif
    free(page);
}
