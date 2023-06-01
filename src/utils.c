#include "utils.h"

#include <stdlib.h>

char *file_read(FILE *file) {
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer = malloc(file_size + 1);
    file_size = fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';
    return buffer;
}
