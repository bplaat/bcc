#ifndef UTILS_MATH_H
#define UTILS_MATH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

size_t align(size_t size, size_t alignment);

bool is_power_of_two(int64_t x);

uint64_t log_two(uint64_t x);

#endif
