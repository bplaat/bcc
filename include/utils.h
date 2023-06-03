#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "lexer.h"

size_t align(size_t size, size_t alignment);

bool power_of_two(int64_t x);

char *strdup(const char *str);

char *strndup(const char *str, size_t size);

char *file_read(FILE *file);

void print_error(char *text, Token *token, char *fmt, ...);

#endif
