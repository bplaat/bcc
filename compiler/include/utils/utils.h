#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "lexer.h"

// System
typedef enum System {
    SYSTEM_MACOS,
    SYSTEM_LINUX,
    SYSTEM_WINDOWS,
} System;

// Arch
typedef enum Arch {
    ARCH_X86_64,
    ARCH_ARM64,
} Arch;

// Strdup pollyfill
char *strdup(const char *str);

char *strndup(const char *str, size_t size);

// I/O helpers
char *file_read(FILE *file);

char *string_format(char *fmt, ...);

void print_error(Token *token, char *fmt, ...);

#endif
