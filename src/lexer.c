#include "lexer.h"

#include <ctype.h>
#include <stdlib.h>

Token lexer_next_token(Lexer *lexer) {
    if (*lexer->c == '\0') return (Token){.type = TOKEN_EOF};

    // Whitespace
    while (isspace(*lexer->c)) lexer->c++;

    // Literals
    if (isdigit(*lexer->c)) {
        return (Token){.type = TOKEN_INTEGER, .integer = strtol(lexer->c, &lexer->c, 10)};
    }

    // Operators
    if (*lexer->c == '+') {
        lexer->c++;
        return (Token){.type = TOKEN_ADD};
    }
    if (*lexer->c == '-') {
        lexer->c++;
        return (Token){.type = TOKEN_SUB};
    }

    // Unknown character
    return (Token){.type = TOKEN_UNKNOWN, .character = *lexer->c};
}
