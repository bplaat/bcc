#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

size_t align(size_t size, size_t alignment) {
    return (size + alignment - 1) / alignment * alignment;
}

char *strdup(const char *string) {
    char *newString = malloc(strlen(string) + 1);
    strcpy(newString, string);
    return newString;
}

char *format(char *fmt, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    return strdup(buffer);
}

char *file_read(char *path) {
    FILE *file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer = malloc(fileSize + 1);
    fileSize = fread(buffer, 1, fileSize, file);
    buffer[fileSize] = '\0';
    fclose(file);
    return buffer;
}

void print_error(char *path, List *lines, int32_t line, int32_t position, char *fmt, ...) {
    fprintf(stderr, "%s:%d:%d ERROR: ", path, line + 1, position + 1);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n%4d | %s\n", line + 1, (char *)list_get(lines, line));
    fprintf(stderr, "     | ");
    for (int32_t i = 0; i < position; i++) fprintf(stderr, " ");
    fprintf(stderr, "^\n");
    exit(EXIT_FAILURE);
}
