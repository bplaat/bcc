#include "parser.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*

type = (INT | LONG | SIGNED | UNSIGNED)+ STAR*
vardef = type VARIABLE (LBRACKET INTEGER RBRACKET)?

program = funcdef*
funcdef = vardef LPAREN (vardef COMMA?)* RPAREN LCURLY statement* RCURLY

block = LCURLY statement* RCURLY | statement
statement = LCURLY? block
    | IF LPAREN assign RPAREN block (ELSE block)?
    | WHILE LPAREN assign RPAREN block
    | FOR LPAREN declarations? SEMICOLON assigns? SEMICOLON assigns? RPAREN block
    | RETURN assign SEMICOLON
    | SEMICOLON
    | declarations SEMICOLON

declarations = vardef (ASSIGN assign)? (COMMA? VARIABLE (ASSIGN assign)?)*
    | assigns
assigns = (assign COMMA?)*
assign = logic (ASSIGN assign)?

logic = equality ((LOGIC_AND | LOGIC_OR) equality)*
equality = relational ((EQ | NEQ) relational)*
relational = add ((LT | LTEQ | GT | GTEQ) add)*
add = mul ((ADD | SUB) mul)*
mul = unary ((STAR | DIV | MOD) unary)*
unary = ADD unary | SUB unary | STAR unary | ADDR unary | LOGIC_NOT unary | primary
primary = LPAREN logic RPAREN | INTEGER | VARIABLE (LPAREN (assign COMMA?)* RPAREN)? | variable
variable = VARIABLE

*/

Arch *arch;
char *text;
List *tokens;
int32_t position;
Node *currentBlock = NULL;

Node *parser(char *_text, List *_tokens, Arch *_arch) {
    text = _text;
    tokens = _tokens;
    position = 0;
    arch = _arch;
    return parser_program();
}

#define current() ((Token *)list_get(tokens, position))
#define previous() ((Token *)list_get(tokens, position - 1))
#define next() ((Token *)list_get(tokens, position + 1))

void parser_check(TokenKind kind) {
    if (current()->kind != kind) {
        fprintf(stderr, "%s\n", text);
        for (int32_t i = 0; i < current()->position; i++) fprintf(stderr, " ");
        fprintf(stderr, "^\nUnexpected: '%s' needed '%s'\n", token_to_string(kind), token_to_string(current()->kind));
        exit(EXIT_FAILURE);
    }
}

void parser_eat(TokenKind kind) {
    if (current()->kind == kind) {
        position++;
    } else {
        parser_check(kind);
    }
}

Type *parser_type(void) {
    Type *type = type_new(TYPE_INTEGER, 4, true);
    if (!(current()->kind == TOKEN_INT || current()->kind == TOKEN_LONG || current()->kind == TOKEN_SIGNED ||
        current()->kind == TOKEN_UNSIGNED)) {
        fprintf(stderr, "%s %d\n", text, position);
        for (int32_t i = 0; i < current()->position; i++) fprintf(stderr, " ");
        fprintf(stderr, "^\nExpected a type keyword\n");
        exit(EXIT_FAILURE);
    }
    while (current()->kind == TOKEN_INT || current()->kind == TOKEN_LONG || current()->kind == TOKEN_SIGNED ||
           current()->kind == TOKEN_UNSIGNED) {
        if (current()->kind == TOKEN_INT) {
            parser_eat(TOKEN_INT);
            type->size = 4;
        }
        if (current()->kind == TOKEN_LONG) {
            parser_eat(TOKEN_LONG);
            type->size = 8;
        }
        if (current()->kind == TOKEN_SIGNED) {
            parser_eat(TOKEN_SIGNED);
            type->isSigned = true;
        }
        if (current()->kind == TOKEN_UNSIGNED) {
            parser_eat(TOKEN_UNSIGNED);
            type->isSigned = false;
        }
    }
    while (current()->kind == TOKEN_STAR) {
        parser_eat(TOKEN_STAR);
        type = type_pointer(type);
    }
    return type;
}

Node *parser_vardef(void) {
    Type *type = parser_type();
    int32_t skipped = 0;
    parser_eat(TOKEN_VARIABLE);
    skipped++;

    if (current()->kind == TOKEN_LBRACKET) {
        parser_eat(TOKEN_LBRACKET);
        skipped++;
        type = type_array(type, current()->integer);
        parser_eat(TOKEN_INTEGER);
        skipped++;
        parser_eat(TOKEN_RBRACKET);
        skipped++;
    }

    position -= skipped;
    Node *variableNode = parser_variable(type);
    position += skipped - 1;
    return variableNode;
}

Node *parser_program(void) {
    Node *programNode = node_new(NODE_PROGRAM);
    programNode->funcs = list_new(4);
    while (current()->kind != TOKEN_EOF) {
        list_add(programNode->funcs, parser_funcdef());
    }
    return programNode;
}

Node *parser_funcdef(void) {
    Type *type = parser_type();
    Node *funcdefNode = node_new_multiple(NODE_FUNCDEF);
    funcdefNode->functionName = current()->string;
    funcdefNode->type = type;
    funcdefNode->locals = list_new(4);
    funcdefNode->argsSize = 0;

    parser_eat(TOKEN_VARIABLE);
    parser_eat(TOKEN_LPAREN);
    if (current()->kind != TOKEN_RPAREN) {
        for (;;) {
            currentBlock = funcdefNode;
            parser_vardef();
            funcdefNode->argsSize++;
            if (current()->kind == TOKEN_COMMA) {
                parser_eat(TOKEN_COMMA);
                continue;
            } else {
                break;
            }
        }
    }
    parser_eat(TOKEN_RPAREN);

    parser_eat(TOKEN_LCURLY);
    while (current()->kind != TOKEN_RCURLY && current()->kind != TOKEN_EOF) {
        currentBlock = funcdefNode;
        list_add(funcdefNode->nodes, parser_statement());
    }
    parser_eat(TOKEN_RCURLY);
    return funcdefNode;
}

Node *parser_block(void) {
    Node *blockNode = node_new_multiple(NODE_BLOCK);
    blockNode->locals = list_new(4);
    blockNode->parentBlock = currentBlock;
    if (current()->kind == TOKEN_LCURLY) {
        parser_eat(TOKEN_LCURLY);
        while (current()->kind != TOKEN_RCURLY && current()->kind != TOKEN_EOF) {
            currentBlock = blockNode;
            list_add(blockNode->nodes, parser_statement());
        }
        parser_eat(TOKEN_RCURLY);
    } else {
        currentBlock = blockNode;
        list_add(blockNode->nodes, parser_statement());
    }
    return blockNode;
}

Node *parser_statement(void) {
    if (current()->kind == TOKEN_LCURLY) {
        return parser_block();
    }

    if (current()->kind == TOKEN_IF) {
        Node *ifNode = node_new(NODE_IF);
        parser_eat(TOKEN_IF);
        parser_eat(TOKEN_LPAREN);
        ifNode->condition = parser_assign();
        parser_eat(TOKEN_RPAREN);
        ifNode->thenBlock = parser_block();
        if (current()->kind == TOKEN_ELSE) {
            parser_eat(TOKEN_ELSE);
            ifNode->elseBlock = parser_block();
        } else {
            ifNode->elseBlock = NULL;
        }
        return ifNode;
    }

    if (current()->kind == TOKEN_WHILE) {
        Node *whileNode = node_new(NODE_WHILE);
        parser_eat(TOKEN_WHILE);
        parser_eat(TOKEN_LPAREN);
        whileNode->condition = parser_assign();
        parser_eat(TOKEN_RPAREN);
        whileNode->thenBlock = parser_block();
        return whileNode;
    }

    if (current()->kind == TOKEN_FOR) {
        Node *forNode = node_new(NODE_WHILE);
        parser_eat(TOKEN_FOR);

        parser_eat(TOKEN_LPAREN);
        if (current()->kind != TOKEN_SEMICOLON) {
            list_add(currentBlock->nodes, parser_declarations());
        }
        parser_eat(TOKEN_SEMICOLON);
        if (current()->kind != TOKEN_SEMICOLON) {
            forNode->condition = parser_assigns();
        } else {
            forNode->condition = NULL;
        }
        parser_eat(TOKEN_SEMICOLON);
        Node *incrementsNode = NULL;
        if (current()->kind != TOKEN_RPAREN) {
            incrementsNode = parser_assigns();
        }
        parser_eat(TOKEN_RPAREN);

        forNode->thenBlock = parser_block();
        if (incrementsNode != NULL) {
            list_add(forNode->thenBlock->nodes, incrementsNode);
        }
        return forNode;
    }

    if (current()->kind == TOKEN_RETURN) {
        parser_eat(TOKEN_RETURN);
        Node *returnNode = node_new_unary(NODE_RETURN, parser_assign());
        parser_eat(TOKEN_SEMICOLON);
        return returnNode;
    }

    if (current()->kind == TOKEN_SEMICOLON) {
        parser_eat(TOKEN_SEMICOLON);
        return node_new(NODE_NULL);
    }

    Node *declarationsNode = parser_declarations();
    parser_eat(TOKEN_SEMICOLON);
    return declarationsNode;
}

Node *parser_declarations(void) {
    if (current()->kind == TOKEN_INT || current()->kind == TOKEN_LONG || current()->kind == TOKEN_SIGNED ||
        current()->kind == TOKEN_UNSIGNED) {
        Node *multipleNode = node_new_multiple(NODE_MULTIPLE);
        Node *variableNode = NULL;
        Type *type;
        for (;;) {
            if (variableNode == NULL) {
                variableNode = parser_vardef();
                type = variableNode->type;
            } else {
                variableNode = parser_variable(type);
            }

            if (current()->kind == TOKEN_ASSIGN) {
                parser_eat(TOKEN_ASSIGN);

                if (variableNode->kind == NODE_DEREF) {
                    list_add(multipleNode->nodes, node_new_operation(NODE_ASSIGN, variableNode->unary, parser_assign()));
                }
                else if (variableNode->kind == NODE_VARIABLE) {
                    list_add(multipleNode->nodes, node_new_operation(NODE_ASSIGN, node_new_unary(NODE_ADDR, variableNode), parser_assign()));
                }
                else {
                    fprintf(stderr, "parser_declarations() TODO\n");
                    exit(1);
                }
            }

            if (current()->kind == TOKEN_COMMA) {
                parser_eat(TOKEN_COMMA);
            } else {
                break;
            }
        }
        return multipleNode;
    }
    return parser_assigns();
}

Node *parser_assigns(void) {
    Node *multipleNode = node_new_multiple(NODE_MULTIPLE);
    for (;;) {
        list_add(multipleNode->nodes, parser_assign());
        if (current()->kind == TOKEN_COMMA) {
            parser_eat(TOKEN_COMMA);
        } else {
            break;
        }
    }
    return multipleNode;
}

Node *parser_assign(void) {
    Node *node = parser_logic();
    if (current()->kind == TOKEN_ASSIGN) {
        parser_eat(TOKEN_ASSIGN);
        if (node->kind == NODE_DEREF) {
            if (node->unary->type->kind == TYPE_ARRAY) {
                // node->unary->type = type_pointer(node->unary->type->base);
                node->unary->type = type_pointer(node->unary->type->base);
                node->unary = node_new_unary(NODE_ADDR, node->unary);
            }
            return node_new_operation(NODE_ASSIGN, node->unary, parser_assign());
        }
        if (node->kind == NODE_VARIABLE) {
            return node_new_operation(NODE_ASSIGN, node_new_unary(NODE_ADDR, node), parser_assign());
        }
        fprintf(stderr, "parser_assign() TODO\n");
        exit(1);
    }
    return node;
}

Node *parser_logic(void) {
    Node *node = parser_equality();
    while (current()->kind == TOKEN_LOGIC_AND || current()->kind == TOKEN_LOGIC_OR) {
        if (current()->kind == TOKEN_LOGIC_AND) {
            parser_eat(TOKEN_LOGIC_AND);
            node = node_new_operation(NODE_LOGIC_AND, node, parser_equality());
        } else if (current()->kind == TOKEN_LOGIC_OR) {
            parser_eat(TOKEN_LOGIC_OR);
            node = node_new_operation(NODE_LOGIC_OR, node, parser_equality());
        }
    }
    return node;
}

Node *parser_equality(void) {
    Node *node = parser_relational();
    while (current()->kind == TOKEN_EQ || current()->kind == TOKEN_NEQ) {
        if (current()->kind == TOKEN_EQ) {
            parser_eat(TOKEN_EQ);
            node = node_new_operation(NODE_EQ, node, parser_relational());
        } else if (current()->kind == TOKEN_NEQ) {
            parser_eat(TOKEN_NEQ);
            node = node_new_operation(NODE_NEQ, node, parser_relational());
        }
    }
    return node;
}

Node *parser_relational(void) {
    Node *node = parser_add();
    while (current()->kind == TOKEN_LT || current()->kind == TOKEN_LTEQ || current()->kind == TOKEN_GT ||
           current()->kind == TOKEN_GTEQ) {
        if (current()->kind == TOKEN_LT) {
            parser_eat(TOKEN_LT);
            node = node_new_operation(NODE_LT, node, parser_add());
        } else if (current()->kind == TOKEN_LTEQ) {
            parser_eat(TOKEN_LTEQ);
            node = node_new_operation(NODE_LTEQ, node, parser_add());
        } else if (current()->kind == TOKEN_GT) {
            parser_eat(TOKEN_GT);
            node = node_new_operation(NODE_GT, node, parser_add());
        } else if (current()->kind == TOKEN_GTEQ) {
            parser_eat(TOKEN_GTEQ);
            node = node_new_operation(NODE_GTEQ, node, parser_add());
        }
    }
    return node;
}

Node *parser_add(void) {
    Node *node = parser_mul();
    while (current()->kind == TOKEN_ADD || current()->kind == TOKEN_SUB) {
        if (current()->kind == TOKEN_ADD) {
            parser_eat(TOKEN_ADD);
            Node *rhs = parser_mul();
            if (node->type->kind == TYPE_POINTER && rhs->type->kind == TYPE_INTEGER) {
                rhs = node_new_operation(NODE_MUL, rhs, node_new_integer(align(node->type->size, arch->stackAlign)));
            }
            if (node->type->kind == TYPE_ARRAY && rhs->type->kind == TYPE_INTEGER) {
                node = node_new_unary(NODE_ADDR, node);
                rhs = node_new_operation(NODE_MUL, rhs, node_new_integer(node->type->base->size));
            }
            node = node_new_operation(NODE_ADD, node, rhs);
        }

        else if (current()->kind == TOKEN_SUB) {
            parser_eat(TOKEN_SUB);
            Node *rhs = parser_mul();
            if (node->type->kind == TYPE_POINTER && rhs->type->kind == TYPE_INTEGER) {
                rhs = node_new_operation(NODE_MUL, rhs, node_new_integer(align(node->type->size, arch->stackAlign)));
            }
            node = node_new_operation(NODE_SUB, node, rhs);
            if (node->type->kind == TYPE_POINTER && rhs->type->kind == TYPE_POINTER) {
                node = node_new_operation(NODE_DIV, node, node_new_integer(align(node->type->size, arch->stackAlign)));
                node->type = node->type->base;
            }
        }
    }
    return node;
}

Node *parser_mul(void) {
    Node *node = parser_unary();
    while (current()->kind == TOKEN_STAR || current()->kind == TOKEN_DIV || current()->kind == TOKEN_MOD) {
        if (current()->kind == TOKEN_STAR) {
            parser_eat(TOKEN_STAR);
            node = node_new_operation(NODE_MUL, node, parser_unary());
        } else if (current()->kind == TOKEN_DIV) {
            parser_eat(TOKEN_DIV);
            node = node_new_operation(NODE_DIV, node, parser_unary());
        } else if (current()->kind == TOKEN_MOD) {
            parser_eat(TOKEN_MOD);
            node = node_new_operation(NODE_MOD, node, parser_unary());
        }
    }
    return node;
}

Node *parser_unary(void) {
    if (current()->kind == TOKEN_ADD) {
        parser_eat(TOKEN_ADD);
        return parser_unary();
    }

    if (current()->kind == TOKEN_SUB) {
        parser_eat(TOKEN_SUB);
        return node_new_unary(NODE_NEG, parser_unary());
    }

    if (current()->kind == TOKEN_ADDR) {
        parser_eat(TOKEN_ADDR);
        Node *addrNode = node_new_unary(NODE_ADDR, parser_unary());
        if (addrNode->unary->kind != NODE_VARIABLE) {
            fprintf(stderr, "%s\n", text);
            for (int32_t i = 0; i < current()->position; i++) fprintf(stderr, " ");
            fprintf(stderr, "^\nYou can only get an address from a variable\n");
            exit(EXIT_FAILURE);
        }
        if (addrNode->unary->type->kind == TYPE_ARRAY) {
            addrNode->type = type_pointer(addrNode->unary->type->base);
        } else {
            addrNode->type = type_pointer(addrNode->unary->type);
        }
        return addrNode;
    }

    if (current()->kind == TOKEN_STAR) {
        parser_eat(TOKEN_STAR);
        Node *derefNode = node_new_unary(NODE_DEREF, parser_unary());
        if (derefNode->unary->type->base == NULL) {
            fprintf(stderr, "%s\n", text);
            for (int32_t i = 0; i < current()->position; i++) fprintf(stderr, " ");
            fprintf(stderr, "^\nYou can't dereference a ");
            type_print(stderr, derefNode->unary->type);
            fprintf(stderr, " type\n");
            exit(EXIT_FAILURE);
        }
        derefNode->type = derefNode->unary->type->base;
        return derefNode;
    }

    if (current()->kind == TOKEN_LOGIC_NOT) {
        parser_eat(TOKEN_LOGIC_NOT);
        return node_new_unary(NODE_LOGIC_NOT, parser_unary());
    }

    return parser_primary();
}

Node *parser_primary(void) {
    if (current()->kind == TOKEN_LPAREN) {
        parser_eat(TOKEN_LPAREN);
        Node *node = parser_logic();
        parser_eat(TOKEN_RPAREN);
        return node;
    }

    if (current()->kind == TOKEN_INTEGER) {
        Node *integerNode = node_new_integer(current()->integer);
        parser_eat(TOKEN_INTEGER);
        return integerNode;
    }

    if (current()->kind == TOKEN_VARIABLE && next()->kind == TOKEN_LPAREN) {
        Node *funccallNode = node_new_multiple(NODE_FUNCCALL);
        funccallNode->type = type_new(TYPE_INTEGER, 4, true);
        funccallNode->functionName = current()->string;
        parser_eat(TOKEN_VARIABLE);
        parser_eat(TOKEN_LPAREN);
        if (current()->kind != TOKEN_RPAREN) {
            for (;;) {
                list_add(funccallNode->nodes, parser_assign());
                if (current()->kind == TOKEN_COMMA) {
                    parser_eat(TOKEN_COMMA);
                } else {
                    break;
                }
            }
        }
        parser_eat(TOKEN_RPAREN);
        return funccallNode;
    }

    if (current()->kind == TOKEN_VARIABLE) {
        return parser_variable(NULL);
    }

    fprintf(stderr, "%s\n", text);
    for (int32_t i = 0; i < current()->position; i++) fprintf(stderr, " ");
    fprintf(stderr, "^\nUnexpected: %s token\n", token_to_string(current()->kind));
    exit(EXIT_FAILURE);
}

Node *parser_variable(Type *type) {
    parser_check(TOKEN_VARIABLE);
    Local *local = node_find_local(currentBlock, current()->string);
    if (local == NULL) {
        if (type != NULL) {
            for (size_t i = 0; i < currentBlock->locals->size; i++) {
                Local *local = list_get(currentBlock->locals, i);
                if (!strcmp(local->name, current()->string)) {
                    fprintf(stderr, "%s\n", text);
                    for (int32_t i = 0; i < current()->position; i++) fprintf(stderr, " ");
                    fprintf(stderr, "^\nCan't redefine variable: %s\n", current()->string);
                    exit(EXIT_FAILURE);
                }
            }

            local = local_new(current()->string, type);
            local->offset = align(local->type->size, arch->stackAlign);
            for (size_t i = 0; i < currentBlock->locals->size; i++) {
                Local *local = list_get(currentBlock->locals, i);
                local->offset += align(local->type->size, arch->stackAlign);
            }
            list_add(currentBlock->locals, local);
        } else {
            fprintf(stderr, "%s\n", text);
            for (int32_t i = 0; i < current()->position; i++) fprintf(stderr, " ");
            fprintf(stderr, "^\nCan't find variable: %s\n", current()->string);
            exit(EXIT_FAILURE);
        }
    }

    Node *variableNode = node_new(NODE_VARIABLE);
    variableNode->local = local;
    variableNode->type = local->type;
    parser_eat(TOKEN_VARIABLE);
    return variableNode;
}
