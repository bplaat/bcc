#include "utils.h"

#include <stdlib.h>
#include <string.h>

size_t align(size_t size, size_t alignment) { return (size + alignment - 1) / alignment * alignment; }

// Strdup pollyfills
char *strdup(const char *str) { return strndup(str, strlen(str)); }

char *strndup(const char *str, size_t size) {
    char *copy = malloc(size + 1);
    if (copy == NULL) return NULL;
    memcpy(copy, str, size);
    copy[size] = '\0';
    return copy;
}

#define FILE_READ_BUFFER_SIZE 512

char *file_read(FILE *file) {
    // Read stdin in chunks because fseek SEEK_END won't work
    if (file == stdin) {
        // Buggy: not well tested
        size_t capacity = FILE_READ_BUFFER_SIZE;
        char *buffer = malloc(capacity);
        char *c = buffer;
        size_t bytes_read;
        while ((bytes_read = fread(c, 1, FILE_READ_BUFFER_SIZE, file)) > 0) {
            c += bytes_read;
            if ((size_t)(c - buffer) + FILE_READ_BUFFER_SIZE > capacity) {
                capacity *= 2;
                buffer = realloc(buffer, capacity);
            }
        }
        return buffer;
    }

    // Read normal file
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer = malloc(file_size + 1);
    file_size = fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';
    return buffer;
}
