#include "parser.h"
#include "utils.h"

Node *parser(Arch *arch, char *path, List *lines, List *tokens) {
    Parser parser;
    parser.arch = arch;
    parser.path = path;
    parser.lines = lines;
    parser.tokens = tokens;
    parser.position = 0;
    return parser_program(&parser);
}

#define current() ((Token *)list_get(parser->tokens, parser->position))
#define next(pos) ((Token *)list_get(parser->tokens, parser->position + 1 + pos))

void parser_eat(Parser *parser, TokenKind kind) {
    if (current()->kind == kind) {
        parser->position++;
    } else {
        print_error(parser->path, parser->lines, current()->line, current()->position,
            "Unexpected: '%s' needed '%s'", token_kind_to_string(current()->kind), token_kind_to_string(kind));
    }
}

Node *parser_program(Parser *parser) {
    Node *programNode = node_new_multiple(NODE_PROGRAM);
    while (current()->kind != TOKEN_EOF) {
        Node *node = parser_statement(parser);
        if (node != NULL) list_add(programNode->nodes, node);
    }
    return programNode;
}

Node *parser_statement(Parser *parser) {
    if (current()->kind == TOKEN_NEWLINE) {
        parser_eat(parser, TOKEN_NEWLINE);
        return NULL;
    }

    if (current()->kind == TOKEN_TIMES) {
        parser_eat(parser, TOKEN_TIMES);
        Node *lhs = parser_assign(parser);
        return node_new_operation(NODE_TIMES, lhs, parser_statement(parser));
    }

    if (current()->kind == TOKEN_SECTION) {
        parser_eat(parser, TOKEN_SECTION);
        Node *node = node_new_string(NODE_SECTION, current()->string);
        parser_eat(parser, TOKEN_KEYWORD);
        return node;
    }

    if (current()->kind == TOKEN_KEYWORD && next(0)->kind == TOKEN_COLON) {
        char *name = current()->string;
        parser_eat(parser, TOKEN_KEYWORD);
        parser_eat(parser, TOKEN_COLON);
        Node *node = node_new_string(NODE_LABEL, name);
        if (current()->kind == TOKEN_NEWLINE) {
            parser_eat(parser, TOKEN_NEWLINE);
        }
        return node;
    }

    if (
        (current()->kind >= TOKEN_TIMES && current()->kind <= TOKEN_DQ) ||
        (current()->kind >= TOKEN_X86_64_NOP && current()->kind <= TOKEN_X86_64_RET)
    ) {
        Node *instructionNode = node_new_multiple(NODE_INSTRUCTION);
        instructionNode->opcode = current()->kind;
        parser_eat(parser, current()->kind);
        while (current()->kind != TOKEN_NEWLINE) {
            Node *node = parser_assign(parser);
            if (node != NULL) list_add(instructionNode->nodes, node);
            if (current()->kind == TOKEN_COMMA) {
                parser_eat(parser, TOKEN_COMMA);
                if (current()->kind == TOKEN_NEWLINE) {
                    parser_eat(parser, TOKEN_NEWLINE);
                }
            } else {
                break;
            }
        }
        parser_eat(parser, TOKEN_NEWLINE);
        return instructionNode;
    }

    Node *node = parser_assign(parser);
    parser_eat(parser, TOKEN_NEWLINE);
    return node;
}

Node *parser_assign(Parser *parser) {
    if (current()->kind == TOKEN_KEYWORD && next(0)->kind == TOKEN_ASSIGN) {
        char *name = current()->string;
        parser_eat(parser, TOKEN_KEYWORD);
        parser_eat(parser, TOKEN_ASSIGN);
        return node_new_operation(NODE_ASSIGN, node_new_string(NODE_KEYWORD, name), parser_assign(parser));
    }
    return parser_add(parser);
}


Node *parser_add(Parser *parser) {
    Node *node = parser_mul(parser);
    while (current()->kind == TOKEN_ADD || current()->kind == TOKEN_SUB) {
        if (current()->kind == TOKEN_ADD) {
            parser_eat(parser, TOKEN_ADD);
            node = node_new_operation(NODE_ADD, node, parser_mul(parser));
        }

        if (current()->kind == TOKEN_SUB) {
            parser_eat(parser, TOKEN_SUB);
            node = node_new_operation(NODE_SUB, node, parser_mul(parser));
        }
    }
    return node;
}

Node *parser_mul(Parser *parser) {
    Node *node = parser_unary(parser);
    while (current()->kind == TOKEN_MUL || current()->kind == TOKEN_DIV || current()->kind == TOKEN_MOD) {
        if (current()->kind == TOKEN_MUL) {
            parser_eat(parser, TOKEN_MUL);
            node = node_new_operation(NODE_MUL, node, parser_unary(parser));
        }
        if (current()->kind == TOKEN_DIV) {
            parser_eat(parser, TOKEN_DIV);
            node = node_new_operation(NODE_DIV, node, parser_unary(parser));
        }
        if (current()->kind == TOKEN_MOD) {
            parser_eat(parser, TOKEN_MOD);
            node = node_new_operation(NODE_MOD, node, parser_unary(parser));
        }
    }
    return node;
}

Node *parser_unary(Parser *parser) {
    if (current()->kind == TOKEN_ADD) {
        parser_eat(parser, TOKEN_ADD);
        return parser_unary(parser);
    }
    if (current()->kind == TOKEN_SUB) {
        parser_eat(parser, TOKEN_SUB);
        Node *node = parser_unary(parser);
        if (node->kind == NODE_INTEGER) {
            node->integer = -node->integer;
            return node;
        }
        return node_new_unary(NODE_NEG, node);
    }
    return parser_primary(parser);
}

Node *parser_primary(Parser *parser) {
    if (current()->kind == TOKEN_LPAREN) {
        parser_eat(parser, TOKEN_LPAREN);
        Node *node = parser_assign(parser);
        parser_eat(parser, TOKEN_RPAREN);
        return node;
    }
    if (current()->kind == TOKEN_INTEGER) {
        Node *node = node_new_integer(current()->integer);
        parser_eat(parser, TOKEN_INTEGER);
        return node;
    }
    if (current()->kind == TOKEN_STRING) {
        Node *node = node_new_string(NODE_STRING, current()->string);
        parser_eat(parser, TOKEN_STRING);
        return node;
    }

    if (
        (current()->kind >= TOKEN_BYTE && current()->kind <= TOKEN_QWORD) ||
        current()->kind == TOKEN_LBRACKET
    ) {
        size_t size = 0;
        if (current()->kind == TOKEN_BYTE) {
            size = 8;
            parser_eat(parser, TOKEN_BYTE);
        }
        else if (current()->kind == TOKEN_WORD) {
            size = 16;
            parser_eat(parser, TOKEN_WORD);
        }
        else if (current()->kind == TOKEN_DWORD) {
            size = 32;
            parser_eat(parser, TOKEN_DWORD);
        }
        else if (current()->kind == TOKEN_QWORD) {
            size = 64;
            parser_eat(parser, TOKEN_QWORD);
        }
        if (current()->kind == TOKEN_PTR) {
            parser_eat(parser, TOKEN_PTR);
        }

        parser_eat(parser, TOKEN_LBRACKET);
        Node *node = node_new_unary(NODE_ADDR, parser_assign(parser));
        node->size = size;
        parser_eat(parser, TOKEN_RBRACKET);
        return node;
    }

    if (current()->kind >= TOKEN_EAX && current()->kind <= TOKEN_RDI) {
        Node *node;
        if (current()->kind >= TOKEN_EAX && current()->kind <= TOKEN_EDI) {
            node = node_new_register(current()->kind - TOKEN_EAX, 32);
        }
        if (current()->kind >= TOKEN_RAX && current()->kind <= TOKEN_RDI) {
            node = node_new_register(current()->kind - TOKEN_RAX, 64);
        }
        parser_eat(parser, current()->kind);
        return node;
    }

    if (current()->kind == TOKEN_KEYWORD) {
        Node *node = node_new_string(NODE_KEYWORD, current()->string);
        parser_eat(parser, TOKEN_KEYWORD);
        return node;
    }

    print_error(parser->path, parser->lines, current()->line, current()->position,
        "Unexpected: '%s'", token_kind_to_string(current()->kind));
    return NULL;
}
