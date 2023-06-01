#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

size_t align(size_t size, size_t alignment);

char *strdup(const char *str);

char *strndup(const char *str, size_t size);

char *file_read(FILE *file);

#endif
