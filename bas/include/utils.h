#pragma once

#include <stddef.h>
#include <stdint.h>
#include "list.h"

size_t align(size_t size, size_t alignment);

char *strdup(const char *string);

char *format(char *fmt, ...);

char *file_read(char *path);

void print_error(char *path, List *lines, int32_t line, int32_t position, char *fmt, ...);
