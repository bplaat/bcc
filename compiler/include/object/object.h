#ifndef OBJECT_H
#define OBJECT_H

#include <stdio.h>

#include "parser.h"
#include "utils/utils.h"

void object_out(char *path, System system, Program *program);

void object_linx_out(FILE *f, Program *program);

#endif
