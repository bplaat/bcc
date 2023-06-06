#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "lexer.h"

// Arch
typedef enum Arch {
    ARCH_X86_64,
    ARCH_ARM64,
} Arch;

// Math
size_t align(size_t size, size_t alignment);

bool is_power_of_two(int64_t x);

uint64_t log_two(uint64_t x);

// Strdup pollyfill
char *strdup(const char *str);

char *strndup(const char *str, size_t size);

// I/O helpers
char *file_read(FILE *file);

char *string_format(char *fmt, ...);

void print_error(Token *token, char *fmt, ...);

#endif
