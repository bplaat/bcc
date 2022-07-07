#include "parser.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*

block = LCURLY statement* RCURLY | statement
statement = LCURLY? block
    | IF LPAREN assign RPAREN block (ELSE block)?
    | WHILE LPAREN assign RPAREN block
    | FOR LPAREN declarations? SEMICOLON assigns? SEMICOLON assigns? RPAREN block
    | RETURN assign SEMICOLON
    | SEMICOLON
    | declarations SEMICOLON

type = (INT | SIGNED | UNSIGNED)+ STAR*
declarations = type (factor (ASSIGN assign)? COMMA?)*
    | assigns
assigns = (assign COMMA?)*
assign = logic (ASSIGN assign)?

logic = equality ((LOGIC_AND | LOGIC_OR) equality)*
equality = relational ((EQ | NEQ) relational)*
relational = add ((LT | LTEQ | GT | GTEQ) add)*
add = mul ((ADD | SUB) mul)*
mul = unary ((STAR | DIV | MOD) unary)*
unary = ADD unary | SUB unary | STAR unary | ADDR unary | LOGIC_NOT unary | primary
primary = LPAREN logic RPAREN | NUMBER | VARIABLE (LPAREN (assign COMMA?)* RPAREN)? | VARIABLE

*/

char *text;
List *tokens;
int32_t position;
Node *currentBlock = NULL;
Type *declarationType = NULL;

Node *parser(char *_text, List *_tokens) {
    text = _text;
    tokens = _tokens;
    position = 0;
    return parser_block();
}

#define current() ((Token *)list_get(tokens, position))
#define previous() ((Token *)list_get(tokens, position - 1))
#define next() ((Token *)list_get(tokens, position + 1))

void parser_eat(TokenKind kind) {
    if (current()->kind == kind) {
        position++;
    } else {
        fprintf(stderr, "%s\n", text);
        for (int32_t i = 0; i < current()->position - 1; i++) fprintf(stderr, " ");
        char wantedTokenBuffer[32];
        token_to_string(kind, wantedTokenBuffer);
        char gotTokenBuffer[32];
        token_to_string(current()->kind, gotTokenBuffer);
        fprintf(stderr, "^\nUnexpected: '%s' needed '%s'\n", gotTokenBuffer, wantedTokenBuffer);
        exit(EXIT_FAILURE);
    }
}

Node *parser_block(void) {
    Node *node = node_new_multiple(NODE_BLOCK);
    node->locals = list_new(4);
    node->parentBlock = currentBlock;
    if (current()->kind == TOKEN_LCURLY) {
        parser_eat(TOKEN_LCURLY);
        while (current()->kind != TOKEN_RCURLY && current()->kind != TOKEN_EOF) {
            currentBlock = node;
            Node *statement = parser_statement();
            list_add(node->nodes, statement);
        }
        parser_eat(TOKEN_RCURLY);
    } else {
        currentBlock = node;
        Node *statement = parser_statement();
        list_add(node->nodes, statement);
    }
    return node;
}

Node *parser_statement(void) {
    if (current()->kind == TOKEN_LCURLY) {
        return parser_block();
    }

    if (current()->kind == TOKEN_IF) {
        Node *node = node_new(NODE_IF);
        parser_eat(TOKEN_IF);
        parser_eat(TOKEN_LPAREN);
        node->condition = parser_assign();
        parser_eat(TOKEN_RPAREN);
        node->thenBlock = parser_block();
        if (current()->kind == TOKEN_ELSE) {
            parser_eat(TOKEN_ELSE);
            node->elseBlock = parser_block();
        } else {
            node->elseBlock = NULL;
        }
        return node;
    }

    if (current()->kind == TOKEN_WHILE) {
        Node *node = node_new(NODE_WHILE);
        parser_eat(TOKEN_WHILE);
        parser_eat(TOKEN_LPAREN);
        node->condition = parser_assign();
        parser_eat(TOKEN_RPAREN);
        node->thenBlock = parser_block();
        return node;
    }

    if (current()->kind == TOKEN_FOR) {
        Node *node = node_new(NODE_WHILE);
        parser_eat(TOKEN_FOR);

        parser_eat(TOKEN_LPAREN);
        if (current()->kind != TOKEN_SEMICOLON) {
            list_add(currentBlock->nodes, parser_declarations());
        }
        parser_eat(TOKEN_SEMICOLON);
        if (current()->kind != TOKEN_SEMICOLON) {
            node->condition = parser_assigns();
        } else {
            node->condition = NULL;
        }
        parser_eat(TOKEN_SEMICOLON);
        Node *increment = NULL;
        if (current()->kind != TOKEN_RPAREN) {
            increment = parser_assigns();
        }
        parser_eat(TOKEN_RPAREN);

        node->thenBlock = parser_block();
        if (increment != NULL) {
            list_add(node->thenBlock->nodes, increment);
        }
        return node;
    }

    if (current()->kind == TOKEN_RETURN) {
        parser_eat(TOKEN_RETURN);
        Node *node = node_new_unary(NODE_RETURN, parser_assign());
        parser_eat(TOKEN_SEMICOLON);
        return node;
    }

    if (current()->kind == TOKEN_SEMICOLON) {
        parser_eat(TOKEN_SEMICOLON);
        return node_new(NODE_NULL);
    }

    Node *node = parser_declarations();
    parser_eat(TOKEN_SEMICOLON);
    return node;
}

Type *parser_type(void) {
    Type *type = type_new(TYPE_NUMBER, 8, true);
    while (current()->kind == TOKEN_INT || current()->kind == TOKEN_SIGNED || current()->kind == TOKEN_UNSIGNED) {
        if (current()->kind == TOKEN_INT) {
            parser_eat(TOKEN_INT);
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

Node *parser_declarations(void) {
    Type *type = NULL;
    if (current()->kind == TOKEN_INT || current()->kind == TOKEN_SIGNED || current()->kind == TOKEN_UNSIGNED) {
        type = parser_type();
    }

    if (type != NULL) {
        Node *node = node_new_multiple(NODE_MULTIPLE);
        for (;;) {
            declarationType = type;
            Node *variable = parser_primary();
            declarationType = NULL;

            if (current()->kind == TOKEN_ASSIGN) {
                parser_eat(TOKEN_ASSIGN);
                list_add(node->nodes, node_new_operation(NODE_ASSIGN, variable, parser_assign()));
            } else {
                list_add(node->nodes, node_new_operation(NODE_ASSIGN, variable, node_new_number(0)));
            }

            if (current()->kind == TOKEN_COMMA) {
                parser_eat(TOKEN_COMMA);
            } else {
                break;
            }
        }
        return node;
    }
    return parser_assigns();
}

Node *parser_assigns(void) {
    Node *node = node_new_multiple(NODE_MULTIPLE);
    for (;;) {
        list_add(node->nodes, parser_assign());
        if (current()->kind == TOKEN_COMMA) {
            parser_eat(TOKEN_COMMA);
        } else {
            break;
        }
    }
    return node;
}

Node *parser_assign(void) {
    Node *node = parser_logic();
    if (current()->kind == TOKEN_ASSIGN) {
        parser_eat(TOKEN_ASSIGN);
        return node_new_operation(NODE_ASSIGN, node, parser_assign());
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
            if (node->type->kind == TYPE_POINTER && rhs->type->kind == TYPE_NUMBER) {
                rhs = node_new_operation(NODE_MUL, rhs, node_new_number(8));
            }
            node = node_new_operation(NODE_ADD, node, rhs);
        }

        else if (current()->kind == TOKEN_SUB) {
            parser_eat(TOKEN_SUB);
            Node *rhs = parser_mul();
            if (node->type->kind == TYPE_POINTER && rhs->type->kind == TYPE_NUMBER) {
                rhs = node_new_operation(NODE_MUL, rhs, node_new_number(8));
            }
            node = node_new_operation(NODE_SUB, node, rhs);
            if (node->type->kind == TYPE_POINTER && rhs->type->kind == TYPE_POINTER) {
                node = node_new_operation(NODE_DIV, node, node_new_number(8));
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
        Node *node = node_new_unary(NODE_ADDR, parser_unary());
        if (node->unary->kind != NODE_VARIABLE) {
            fprintf(stderr, "%s\n", text);
            for (int32_t i = 0; i < current()->position - 1; i++) fprintf(stderr, " ");
            fprintf(stderr, "^\nYou can only get an address from a variable\n");
            exit(EXIT_FAILURE);
        }
        node->type = type_pointer(node->unary->type);
        return node;
    }

    if (current()->kind == TOKEN_STAR) {
        parser_eat(TOKEN_STAR);
        Node *node = node_new_unary(NODE_DEREF, parser_unary());
        if (node->unary->type->kind != TYPE_POINTER) {
            fprintf(stderr, "%s\n", text);
            for (int32_t i = 0; i < current()->position - 1; i++) fprintf(stderr, " ");
            fprintf(stderr, "^\nYou can only dereference a pointer type\n");
            exit(EXIT_FAILURE);
        }
        node->type = node->unary->type->base;
        return node;
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

    if (current()->kind == TOKEN_NUMBER) {
        Node *node = node_new_number(current()->number);
        parser_eat(TOKEN_NUMBER);
        return node;
    }

    if (current()->kind == TOKEN_VARIABLE) {
        if (next()->kind == TOKEN_LPAREN) {
            Node *node = node_new_multiple(NODE_FNCALL);
            node->type = type_new(TYPE_NUMBER, 8, true);
            node->string = current()->string;
            parser_eat(TOKEN_VARIABLE);
            parser_eat(TOKEN_LPAREN);
            for (;;) {
                list_add(node->nodes, parser_assign());
                if (current()->kind == TOKEN_COMMA) {
                    parser_eat(TOKEN_COMMA);
                } else {
                    break;
                }
            }
            parser_eat(TOKEN_RPAREN);
            return node;
        }

        Local *local = node_find_local(currentBlock, current()->string);
        if (local == NULL) {
            if (declarationType != NULL) {
                for (size_t i = 0; i < currentBlock->locals->size; i++) {
                    Local *local = list_get(currentBlock->locals, i);
                    if (!strcmp(local->name, current()->string)) {
                        fprintf(stderr, "%s\n", text);
                        for (int32_t i = 0; i < current()->position; i++) fprintf(stderr, " ");
                        fprintf(stderr, "^\nCan't redefine variable: %s\n", current()->string);
                        exit(EXIT_FAILURE);
                    }
                }

                local = malloc(sizeof(Local));
                local->name = current()->string;
                local->type = declarationType;
                local->offset = local->type->size;
                for (size_t i = 0; i < currentBlock->locals->size; i++) {
                    Local *local = list_get(currentBlock->locals, i);
                    local->offset += local->type->size;
                }
                list_add(currentBlock->locals, local);
            } else {
                fprintf(stderr, "%s\n", text);
                for (int32_t i = 0; i < current()->position - 1; i++) fprintf(stderr, " ");
                fprintf(stderr, "^\nCan't find variable: %s\n", current()->string);
                exit(EXIT_FAILURE);
            }
        }

        Node *node = node_new(NODE_VARIABLE);
        node->type = local->type;
        node->string = local->name;
        parser_eat(TOKEN_VARIABLE);
        return node;
    }

    fprintf(stderr, "%s\n", text);
    for (int32_t i = 0; i < current()->position; i++) fprintf(stderr, " ");
    char tokenBuffer[32];
    token_to_string(current()->kind, tokenBuffer);
    fprintf(stderr, "^\nUnexpected: %s token\n", tokenBuffer);
    exit(EXIT_FAILURE);
}
