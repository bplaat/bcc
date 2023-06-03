#include "utils.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// Math
size_t align(size_t size, size_t align) { return (size + align - 1) / align * align; }

bool power_of_two(int64_t x) {
    if (x <= 0) {
        return false;
    }
    return (x & (x - 1)) == 0;
}

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

// Print error
void print_error(char *text, Token *token, char *fmt, ...) {
    fprintf(stderr, "stdin:%d:%d ERROR: ", token->line, token->column);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    // Seek to the right line in text
    char *c = text;
    for (int32_t i = 0; i < token->line - 1; i++) {
        while (*c != '\n' && *c != '\r') c++;
        if (*c == '\r') c++;
        c++;
    }
    char *line_start = c;
    while (*c != '\n' && *c != '\r' && *c != '\0') c++;
    int32_t line_length = c - line_start;

    fprintf(stderr, "\n%4d | ", token->line);
    fwrite(line_start, 1, line_length, stderr);
    fprintf(stderr, "\n     | ");
    for (int32_t i = 0; i < token->column - 1; i++) fprintf(stderr, " ");
    fprintf(stderr, "^\n");
}
