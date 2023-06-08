#include "utils/math.h"

size_t align(size_t size, size_t align) { return (size + align - 1) / align * align; }

bool is_power_of_two(int64_t x) {
    if (x <= 0) {
        return false;
    }
    return (x & (x - 1)) == 0;
}

uint64_t log_two(uint64_t x) {
    uint64_t result = 0;
    while (x >>= 1) result++;
    return result;
}
