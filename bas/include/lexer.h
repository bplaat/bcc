#pragma once

#include "token.h"
#include "list.h"
#include "object.h"

typedef struct Keyword {
    char *keyword;
    TokenKind kind;
} Keyword;

List *lexer(Arch *arch, char *path, List *lines);
