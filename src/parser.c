#include "parser.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Node *parser(Arch *arch, char *text, List *tokens) {
    Parser parser;
    parser.arch = arch;
    parser.text = text;
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
        fprintf(stderr, "%s\n", parser->text);
        for (size_t i = 0; i < current()->position; i++) fprintf(stderr, " ");
        fprintf(stderr, "^\nUnexpected: '%s' needed '%s'\n", token_kind_to_string(current()->kind), token_kind_to_string(kind));
        exit(EXIT_FAILURE);
    }
}

Type *parser_type(Parser *parser) {
    if (!token_kind_is_type(current()->kind)) {
        fprintf(stderr, "%s\n", parser->text);
        for (size_t i = 0; i < current()->position; i++) fprintf(stderr, " ");
        fprintf(stderr, "^\nExpected a type keyword\n");
        exit(EXIT_FAILURE);
    }
    Type *type = type_new(TYPE_INTEGER, parser->arch->wordSize, false);
    while (token_kind_is_type(current()->kind)) {
        if (current()->kind == TOKEN_CHAR) {
            parser_eat(parser, TOKEN_CHAR);
            type->size = 1;
        }
        if (current()->kind == TOKEN_SHORT) {
            parser_eat(parser, TOKEN_SHORT);
            type->size = 2;
        }
        if (current()->kind == TOKEN_INT) {
            parser_eat(parser, TOKEN_INT);
            type->size = 4;
        }
        if (current()->kind == TOKEN_LONG) {
            parser_eat(parser, TOKEN_LONG);
            type->size = 8;
        }
        if (current()->kind == TOKEN_SIGNED) {
            parser_eat(parser, TOKEN_SIGNED);
            type->isUnsigned = false;
        }
        if (current()->kind == TOKEN_UNSIGNED) {
            parser_eat(parser, TOKEN_UNSIGNED);
            type->isUnsigned = true;
        }
    }
    while (current()->kind == TOKEN_STAR) {
        parser_eat(parser, TOKEN_STAR);
        type = type_new_pointer(type);
    }
    return type;
}

Type *parser_type_suffix(Parser *parser, Type *type) {
    if (current()->kind == TOKEN_LBRACKET) {
        parser_eat(parser, TOKEN_LBRACKET);
        int32_t size = current()->integer;
        parser_eat(parser, TOKEN_INTEGER);
        parser_eat(parser, TOKEN_RBRACKET);
        type = type_new_array(parser_type_suffix(parser, type), size);
    }
    return type;
}

Local *parser_find_local(Parser *parser, char *name) {
    Local *local = NULL;
    for (size_t i = 0; i < parser->currentFuncdef->locals->size; i++) {
        Local *otherLocal = list_get(parser->currentFuncdef->locals, i);
        if (!strcmp(otherLocal->name, name)) {
            local = otherLocal;
            break;
        }
    }
    if (local == NULL) {
        fprintf(stderr, "%s\n", parser->text);
        for (size_t i = 0; i < current()->position; i++) fprintf(stderr, " ");
        fprintf(stderr, "^\nCan't find local variable: '%s'\n", name);
        exit(EXIT_FAILURE);
    }
    return local;
}

Node *parser_program(Parser *parser) {
    Node *programNode = node_new_multiple(NODE_PROGRAM);
    while (current()->kind != TOKEN_EOF) {
        Node *funcdefNode = node_new_multiple(NODE_FUNCDEF);
        funcdefNode->type = parser_type(parser);
        funcdefNode->funcname = current()->string;
        funcdefNode->locals = list_new(8);
        funcdefNode->isLeaf = true;
        parser_eat(parser, TOKEN_VARIABLE);
        parser_eat(parser, TOKEN_LPAREN);
        if (current()->kind != TOKEN_RPAREN) {
            for (;;) {
                Type *type = parser_type(parser);
                char *name = current()->string;
                parser_eat(parser, TOKEN_VARIABLE);

                Local *local = local_new(name, type, align(type->size, parser->arch->wordSize));
                for (size_t i = 0; i < funcdefNode->locals->size; i++) {
                    Local *otherLocal = list_get(funcdefNode->locals, i);
                    otherLocal->offset += local->offset;
                }
                list_add(funcdefNode->locals, local);
                funcdefNode->argsSize++;

                if (current()->kind == TOKEN_COMMA) {
                    parser_eat(parser, TOKEN_COMMA);
                } else {
                    break;
                }
            }
        }
        parser_eat(parser, TOKEN_RPAREN);

        parser_eat(parser, TOKEN_LCURLY);
        parser->currentFuncdef = funcdefNode;
        while (current()->kind != TOKEN_RCURLY) {
            Node *node = parser_statement(parser);
            if (node != NULL) list_add(funcdefNode->nodes, node);
        }
        parser_eat(parser, TOKEN_RCURLY);
        list_add(programNode->nodes, funcdefNode);
    }
    return programNode;
}

Node *parser_block(Parser *parser) {
    Node *blockNode = node_new_multiple(NODE_BLOCK);
    if (current()->kind == TOKEN_LCURLY) {
        parser_eat(parser, TOKEN_LCURLY);
        while (current()->kind != TOKEN_RCURLY) {
            Node *node = parser_statement(parser);
            if (node != NULL) list_add(blockNode->nodes, node);
        }
        parser_eat(parser, TOKEN_RCURLY);
    } else {
        Node *node = parser_statement(parser);
        if (node != NULL) list_add(blockNode->nodes, node);
    }
    return blockNode;
}

Node *parser_statement(Parser *parser) {
    if (current()->kind == TOKEN_SEMICOLON) {
        parser_eat(parser, TOKEN_SEMICOLON);
        return NULL;
    }

    if (current()->kind == TOKEN_LCURLY) {
        return parser_block(parser);
    }

    if (current()->kind == TOKEN_IF) {
        Node *node = node_new(NODE_IF);
        parser_eat(parser, TOKEN_IF);
        parser_eat(parser, TOKEN_LPAREN);
        node->condition = parser_assigns(parser);
        parser_eat(parser, TOKEN_RPAREN);
        node->thenBlock = parser_block(parser);
        if (current()->kind == TOKEN_ELSE) {
            parser_eat(parser, TOKEN_ELSE);
            node->elseBlock = parser_block(parser);
        } else {
            node->elseBlock = NULL;
        }
        return node;
    }

    if (current()->kind == TOKEN_WHILE) {
        Node *node = node_new(NODE_WHILE);
        parser_eat(parser, TOKEN_WHILE);
        parser_eat(parser, TOKEN_LPAREN);
        node->condition = parser_assigns(parser);
        parser_eat(parser, TOKEN_RPAREN);
        node->thenBlock = parser_block(parser);
        return node;
    }

    if (current()->kind == TOKEN_FOR) {
        Node *blockNode = node_new_multiple(NODE_BLOCK);

        Node *node = node_new(NODE_WHILE);
        parser_eat(parser, TOKEN_FOR);
        parser_eat(parser, TOKEN_LPAREN);
        if (current()->kind != TOKEN_SEMICOLON) {
            list_add(blockNode->nodes, parser_decls(parser));
        }
        parser_eat(parser, TOKEN_SEMICOLON);
        if (current()->kind != TOKEN_SEMICOLON) {
            node->condition = parser_assigns(parser);
        } else {
            node->condition = NULL;
        }
        parser_eat(parser, TOKEN_SEMICOLON);
        Node *incNode = NULL;
        if (current()->kind != TOKEN_RPAREN) {
            incNode = parser_assigns(parser);
        }
        parser_eat(parser, TOKEN_RPAREN);

        node->thenBlock = parser_block(parser);
        if (incNode != NULL) {
            list_add(node->thenBlock->nodes, incNode);
        }
        list_add(blockNode->nodes, node);

        if (blockNode->nodes->size == 1) {
            return list_get(blockNode->nodes, 0);
        }
        return blockNode;
    }

    if (current()->kind == TOKEN_RETURN) {
        parser_eat(parser, TOKEN_RETURN);
        Node *node = node_new_unary(NODE_RETURN, parser_logic(parser));
        parser_eat(parser, TOKEN_SEMICOLON);
        return node;
    }

    Node *node = parser_decls(parser);
    parser_eat(parser, TOKEN_SEMICOLON);
    return node;
}

Node *parser_decls(Parser *parser) {
    if (token_kind_is_type(current()->kind)) {
        Type *rootType = parser_type(parser);
        Node *blockNode = node_new_multiple(NODE_BLOCK);
        for (;;) {
            Type *type = rootType;

            while (current()->kind == TOKEN_STAR) {
                parser_eat(parser, TOKEN_STAR);
                type = type_new_pointer(type);
            }
            char *name = current()->string;

            Local *local = NULL;
            for (size_t i = 0; i < parser->currentFuncdef->locals->size; i++) {
                Local *otherLocal = list_get(parser->currentFuncdef->locals, i);
                if (!strcmp(otherLocal->name, name)) {
                    local = otherLocal;
                    break;
                }
            }
            if (local != NULL) {
                fprintf(stderr, "%s\n", parser->text);
                for (size_t i = 0; i < current()->position; i++) fprintf(stderr, " ");
                fprintf(stderr, "^\nCan't redefine local variable: '%s'\n", name);
                exit(EXIT_FAILURE);
            }
            parser_eat(parser, TOKEN_VARIABLE);
            type = parser_type_suffix(parser, type);

            local = local_new(name, type, align(type->size, parser->arch->wordSize));
            for (size_t i = 0; i < parser->currentFuncdef->locals->size; i++) {
                Local *otherLocal = list_get(parser->currentFuncdef->locals, i);
                otherLocal->offset += local->offset;
            }
            list_add(parser->currentFuncdef->locals, local);

            if (current()->kind == TOKEN_ASSIGN) {
                parser_eat(parser, TOKEN_ASSIGN);
                list_add(blockNode->nodes, node_new_operation(NODE_ASSIGN, node_new_local(local), parser_assign(parser)));
            }
            if (current()->kind == TOKEN_COMMA) {
                parser_eat(parser, TOKEN_COMMA);
            } else {
                break;
            }
        }
        if (blockNode->nodes->size == 0) {
            return NULL;
        } else if (blockNode->nodes->size == 1) {
            return list_get(blockNode->nodes, 0);
        }
        return blockNode;
    }
    return parser_assigns(parser);
}

Node *parser_assigns(Parser *parser) {
    Node *blockNode = node_new_multiple(NODE_BLOCK);
    for (;;) {
        Node *node = parser_assign(parser);
        list_add(blockNode->nodes, node);

        if (node->kind != NODE_ASSIGN) {
            break;
        }
        if (current()->kind == TOKEN_COMMA) {
            parser_eat(parser, TOKEN_COMMA);
        } else {
            break;
        }
    }
    if (blockNode->nodes->size == 0) {
        return NULL;
    } else if (blockNode->nodes->size == 1) {
        return list_get(blockNode->nodes, 0);
    }
    return blockNode;
}

Node *parser_assign(Parser *parser) {
    Node *lhs = parser_logic(parser);
    if (current()->kind == TOKEN_ASSIGN) {
        parser_eat(parser, TOKEN_ASSIGN);
        if (lhs->kind == NODE_DEREF) {
            return node_new_operation(NODE_ASSIGN_PTR, lhs->unary, parser_assign(parser));
        }
        return node_new_operation(NODE_ASSIGN, lhs, parser_assign(parser));
    }
    return lhs;
}

Node *parser_logic(Parser *parser) {
    Node *node = parser_equality(parser);
    while (current()->kind == TOKEN_LOGIC_OR || current()->kind == TOKEN_LOGIC_AND) {
        if (current()->kind == TOKEN_LOGIC_OR) {
            parser_eat(parser, TOKEN_LOGIC_OR);
            node = node_new_operation(NODE_LOGIC_OR, node, parser_equality(parser));
        }
        if (current()->kind == TOKEN_LOGIC_AND) {
            parser_eat(parser, TOKEN_LOGIC_AND);
            node = node_new_operation(NODE_LOGIC_AND, node, parser_equality(parser));
        }
    }
    return node;
}

Node *parser_equality(Parser *parser) {
    Node *node = parser_relational(parser);
    while (current()->kind == TOKEN_EQ || current()->kind == TOKEN_NEQ) {
        if (current()->kind == TOKEN_EQ) {
            parser_eat(parser, TOKEN_EQ);
            node = node_new_operation(NODE_EQ, node, parser_relational(parser));
        }
        if (current()->kind == TOKEN_NEQ) {
            parser_eat(parser, TOKEN_NEQ);
            node = node_new_operation(NODE_NEQ, node, parser_relational(parser));
        }
    }
    return node;
}

Node *parser_relational(Parser *parser) {
    Node *node = parser_add(parser);
    while (current()->kind == TOKEN_LT || current()->kind == TOKEN_LTEQ || current()->kind == TOKEN_GT || current()->kind == TOKEN_GTEQ) {
        if (current()->kind == TOKEN_LT) {
            parser_eat(parser, TOKEN_LT);
            node = node_new_operation(NODE_LT, node, parser_add(parser));
        }
        if (current()->kind == TOKEN_LTEQ) {
            parser_eat(parser, TOKEN_LTEQ);
            node = node_new_operation(NODE_LTEQ, node, parser_add(parser));
        }
        if (current()->kind == TOKEN_GT) {
            parser_eat(parser, TOKEN_GT);
            node = node_new_operation(NODE_GT, node, parser_add(parser));
        }
        if (current()->kind == TOKEN_GTEQ) {
            parser_eat(parser, TOKEN_GTEQ);
            node = node_new_operation(NODE_GTEQ, node, parser_add(parser));
        }
    }
    return node;
}

Node *parser_add(Parser *parser) {
    Node *node = parser_mul(parser);
    while (current()->kind == TOKEN_ADD || current()->kind == TOKEN_SUB) {
        if (current()->kind == TOKEN_ADD) {
            parser_eat(parser, TOKEN_ADD);
            Node *rhs = parser_mul(parser);
            if ((node->type->kind == TYPE_POINTER || node->type->kind == TYPE_ARRAY) && rhs->type->kind == TYPE_INTEGER && node->type->base->size > 1) {
                rhs = node_new_operation(NODE_MUL, rhs, node_new_integer(node->type->base->size, parser->arch->pointerSize, true));
            }
            node = node_new_operation(NODE_ADD, node, rhs);
        }

        if (current()->kind == TOKEN_SUB) {
            parser_eat(parser, TOKEN_SUB);
            Node *rhs = parser_mul(parser);
            if ((node->type->kind == TYPE_POINTER || node->type->kind == TYPE_ARRAY) && rhs->type->kind == TYPE_INTEGER && node->type->base->size > 1) {
                rhs = node_new_operation(NODE_MUL, rhs, node_new_integer(node->type->base->size, parser->arch->pointerSize, true));
            }
            node = node_new_operation(NODE_SUB, node, rhs);
            if (node->type->kind == TYPE_POINTER && rhs->type->kind == TYPE_POINTER && node->type->base->size > 1) {
                node = node_new_operation(NODE_DIV, node, node_new_integer(node->type->base->size, parser->arch->pointerSize, true));
                node->type = node->type->base; // ?
            }
        }
    }
    return node;
}

Node *parser_mul(Parser *parser) {
    Node *node = parser_unary(parser);
    while (current()->kind == TOKEN_STAR || current()->kind == TOKEN_DIV || current()->kind == TOKEN_MOD) {
        if (current()->kind == TOKEN_STAR) {
            parser_eat(parser, TOKEN_STAR);
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
    if (current()->kind == TOKEN_LOGIC_NOT) {
        parser_eat(parser, TOKEN_LOGIC_NOT);
        return node_new_unary(NODE_LOGIC_NOT, parser_unary(parser));
    }
    if (current()->kind == TOKEN_AND) {
        parser_eat(parser, TOKEN_AND);
        Node *node = node_new_unary(NODE_REF, parser_unary(parser));
        if (node->unary->kind != NODE_LOCAL) {
            fprintf(stderr, "%s\n", parser->text);
            for (size_t i = 0; i < current()->position; i++) fprintf(stderr, " ");
            fprintf(stderr, "^\nYou can only reference a variable\n");
            exit(EXIT_FAILURE);
        }
        if (node->unary->type->kind == TYPE_ARRAY) {
            node->type = type_new_pointer(node->unary->type->base);
        } else {
            node->type = type_new_pointer(node->unary->type);
        }
        return node;
    }
    if (current()->kind == TOKEN_STAR) {
        parser_eat(parser, TOKEN_STAR);
        Node *node = node_new_unary(NODE_DEREF, parser_unary(parser));
        if (node->unary->type->kind != TYPE_POINTER && node->unary->type->kind != TYPE_ARRAY) {

            printf("%s\n", node_to_string(node->unary));

            fprintf(stderr, "%s\n", parser->text);
            for (size_t i = 0; i < current()->position; i++) fprintf(stderr, " ");
            fprintf(stderr, "^\nYou can't dereference a %s type\n", type_to_string(node->unary->type));
            exit(EXIT_FAILURE);
        }
        node->type = node->unary->type->base;
        return node;
    }
    if (current()->kind == TOKEN_SIZEOF) {
        parser_eat(parser, TOKEN_SIZEOF);
        return node_new_integer(parser_unary(parser)->type->size, parser->arch->pointerSize, true);
    }
    return parser_primary_suffix(parser);
}

Node *parser_primary_suffix(Parser *parser) {
    Node *node = parser_primary(parser);
    while (current()->kind == TOKEN_LBRACKET) {
        parser_eat(parser, TOKEN_LBRACKET);
        Node *index = parser_logic(parser);
        if (node->kind == NODE_INTEGER && (index->type->kind == TYPE_POINTER || index->type->kind == TYPE_ARRAY) && index->type->base->size > 1) {
            node = node_new_operation(NODE_MUL, node, node_new_integer(index->type->base->size, parser->arch->pointerSize, true));
        }
        if ((node->type->kind == TYPE_POINTER || node->type->kind == TYPE_ARRAY) && node->type->base->size > 1  && index->kind == NODE_INTEGER) {
            index = node_new_operation(NODE_MUL, index, node_new_integer(node->type->base->size, parser->arch->pointerSize, true));
        }
        node = node_new_unary(NODE_DEREF, node_new_operation(NODE_ADD, node, index));
        node->type = node->unary->type->base;
        parser_eat(parser, TOKEN_RBRACKET);
    }
    return node;
}

Node *parser_primary(Parser *parser) {
    if (current()->kind == TOKEN_LPAREN) {
        parser_eat(parser, TOKEN_LPAREN);
        Node *node = parser_assign(parser);
        parser_eat(parser, TOKEN_RPAREN);
        return node;
    }
    if (current()->kind == TOKEN_INTEGER) {
        Node *node = node_new_integer(current()->integer, parser->arch->wordSize, false);
        parser_eat(parser, TOKEN_INTEGER);
        return node;
    }
    if (current()->kind == TOKEN_CHARACTER) {
        Node *node = node_new_integer(current()->integer, 1, false);
        parser_eat(parser, TOKEN_CHARACTER);
        return node;
    }
    if (current()->kind == TOKEN_VARIABLE) {
        if (next(0)->kind == TOKEN_LPAREN) {
            parser->currentFuncdef->isLeaf = false;
            Node *node = node_new_multiple(NODE_FUNCCALL);
            node->type = type_new(TYPE_INTEGER, parser->arch->wordSize, false);
            node->funcname = current()->string;
            parser_eat(parser, TOKEN_VARIABLE);
            parser_eat(parser, TOKEN_LPAREN);
            while (current()->kind != TOKEN_RPAREN) {
                list_add(node->nodes, parser_assign(parser));
                if (current()->kind == TOKEN_COMMA) {
                    parser_eat(parser, TOKEN_COMMA);
                } else {
                    break;
                }
            }
            parser_eat(parser, TOKEN_RPAREN);
            return node;
        }

        Local *local = parser_find_local(parser, current()->string);
        parser_eat(parser, TOKEN_VARIABLE);
        return node_new_local(local);
    }

    fprintf(stderr, "%s\n", parser->text);
    for (size_t i = 0; i < current()->position; i++) fprintf(stderr, " ");
    fprintf(stderr, "^\nUnexpected: '%s'\n", token_kind_to_string(current()->kind));
    exit(EXIT_FAILURE);
}
