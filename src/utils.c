#include "utils.h"

#include <stdlib.h>

#define CHUNK_SIZE 512

char *file_read(FILE *file) {
    // Read stdin in chunks because fseek SEEK_END won't work
    if (file == stdin) {
        // Buggy: not well tested
        size_t capacity = CHUNK_SIZE;
        char *buffer = malloc(capacity);
        char *c = buffer;
        size_t bytes_read;
        while ((bytes_read = fread(c, 1, CHUNK_SIZE, file)) > 0) {
            c += bytes_read;
            if ((size_t)(c - buffer) + CHUNK_SIZE > capacity) {
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
