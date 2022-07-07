#include "parser.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Node *node_new(NodeKind kind) {
    Node *node = malloc(sizeof(Node));
    node->kind = kind;
    return node;
}

Node *node_new_number(int64_t number) {
    Node *node = malloc(sizeof(Node));
    node->kind = NODE_NUMBER;
    node->number = number;
    node->type = type_new(TYPE_NUMBER);
    return node;
}

Node *node_new_unary(NodeKind kind, Node *unary) {
    Node *node = malloc(sizeof(Node));
    node->kind = kind;
    node->unary = unary;
    node->type = unary->type;
    return node;
}

Node *node_new_operation(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = malloc(sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    node->type = lhs->type;
    return node;
}

void node_print(FILE *file, Node *node) {
    if (node->kind == NODE_BLOCK) {
        fprintf(file, "{ ");
        for (size_t i = 0; i < node->locals->size; i++) {
            Local *local = list_get(node->locals, i);
            fprintf(file, "i64 %s; ", local->name);
        }
        for (size_t i = 0; i < node->statements->size; i++) {
            node_print(file, list_get(node->statements, i));
            fprintf(file, "; ");
        }
        fprintf(file, " }");
        return;
    }

    if (node->kind == NODE_NUMBER) {
        fprintf(file, "%lld", node->number);
    }
    if (node->kind == NODE_VARIABLE) {
        fprintf(file, "%s", node->string);
    }

    if (node->kind == NODE_IF) {
        fprintf(file, "if (");
        type_print(file, node->condition->type);
        node_print(file, node->condition);
        fprintf(file, ") ");
        node_print(file, node->thenBlock);
        if (node->elseBlock != NULL) {
            fprintf(file, " else ");
            node_print(file, node->elseBlock);
        }
    }

    if (node->kind == NODE_WHILE) {
        fprintf(file, "while (");
        if (node->condition != NULL) {
            type_print(file, node->condition->type);
            node_print(file, node->condition);
        } else {
            fprintf(file, "1");
        }
        fprintf(file, ") ");
        node_print(file, node->thenBlock);
    }

    if (node->kind == NODE_RETURN) {
        fprintf(file, "return ");
        type_print(file, node->unary->type);
        node_print(file, node->unary);
    }

    if (node->kind >= NODE_NEG && node->kind <= NODE_LOGIC_NOT) {
        if (node->kind == NODE_NEG) fprintf(file, "(- ");
        if (node->kind == NODE_ADDR) fprintf(file, "(& ");
        if (node->kind == NODE_DEREF) fprintf(file, "(* ");
        if (node->kind == NODE_LOGIC_NOT) fprintf(file, "(! ");
        type_print(file, node->unary->type);
        node_print(file, node->unary);
        fprintf(file, ")");
    }

    if (node->kind >= NODE_ASSIGN && node->kind <= NODE_LOGIC_OR) {
        fprintf(file, "(");
        type_print(file, node->lhs->type);
        node_print(file, node->lhs);
        if (node->kind == NODE_ASSIGN) fprintf(file, " = ");
        if (node->kind == NODE_ADD) fprintf(file, " + ");
        if (node->kind == NODE_SUB) fprintf(file, " - ");
        if (node->kind == NODE_MUL) fprintf(file, " * ");
        if (node->kind == NODE_DIV) fprintf(file, " / ");
        if (node->kind == NODE_MOD) fprintf(file, " %% ");
        if (node->kind == NODE_EQ) fprintf(file, " == ");
        if (node->kind == NODE_NEQ) fprintf(file, " != ");
        if (node->kind == NODE_LT) fprintf(file, " < ");
        if (node->kind == NODE_LTEQ) fprintf(file, " <= ");
        if (node->kind == NODE_GT) fprintf(file, " > ");
        if (node->kind == NODE_GTEQ) fprintf(file, " >= ");
        if (node->kind == NODE_LOGIC_AND) fprintf(file, " && ");
        if (node->kind == NODE_LOGIC_OR) fprintf(file, " || ");
        type_print(file, node->rhs->type);
        node_print(file, node->rhs);
        fprintf(file, ")");
    }
}

/*

block = LCURLY statement* RCURLY | statement
statement = LCURLY? block
    | IF LPAREN assign RPAREN block (ELSE block)?
    | WHILE LPAREN assign RPAREN block
    | FOR LPAREN assign? SEMICOLON assign? SEMICOLON assign? RPAREN block
    | RETURN assign SEMICOLON
    | SEMICOLON
    | assign SEMICOLON
assign = logic (ASSIGN assign)?

logic = equality ((LOGIC_AND | LOGIC_OR) equality)*
equality = relational ((EQ | NEQ) relational)*
relational = add ((LT | LTEQ | GT | GTEQ) add)*
add = mul ((ADD | SUB) mul)*
mul = unary ((STAR | DIV | MOD) unary)*
unary = ADD unary | SUB unary | STAR unary | ADDR unary | LOGIC_NOT unary | primary
primary = LPAREN logic RPAREN | NUMBER | VARIABLE

*/

char *text;
List *tokens;
int32_t position;
Node *currentBlock = NULL;

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
        fprintf(stderr, "%d", current()->kind);
        fprintf(stderr, "%s\n", text);
        for (int32_t i = 0; i < current()->position; i++) fprintf(stderr, " ");
        char wantedTokenBuffer[32];
        token_to_string(kind, wantedTokenBuffer);
        char gotTokenBuffer[32];
        token_to_string(current()->kind, gotTokenBuffer);
        fprintf(stderr, "^\nUnexpected: '%s' needed '%s'\n", gotTokenBuffer, wantedTokenBuffer);
        exit(EXIT_FAILURE);
    }
}

Local *block_find_local(Node *block, char *name) {
    if (block->parentBlock != NULL) {
        Local *local = block_find_local(block->parentBlock, name);
        if (local != NULL) {
            return local;
        }
    }

    for (size_t i = 0; i < block->locals->size; i++) {
        Local *local = list_get(block->locals, i);
        if (!strcmp(local->name, name)) {
            return local;
        }
    }
    return NULL;
}

void block_create_locals(Node *node) {
    static bool inAssign = false;

    if (node->kind == NODE_ASSIGN) {
        inAssign = true;
        block_create_locals(node->lhs);
        block_create_locals(node->rhs);
        inAssign = false;
    }

    if (node->kind >= NODE_NEG && node->kind <= NODE_LOGIC_NOT) {
        block_create_locals(node->unary);
    }

    if (node->kind >= NODE_ADD && node->kind <= NODE_LOGIC_OR) {
        block_create_locals(node->lhs);
        block_create_locals(node->rhs);
    }

    if (node->kind == NODE_VARIABLE) {
        Local *local = block_find_local(currentBlock, node->string);
        if (local == NULL) {
            if (inAssign) {
                Local *local = malloc(sizeof(Local));
                local->name = node->string;
                local->size = 8;
                local->offset = local->size;
                for (size_t i = 0; i < currentBlock->locals->size; i++) {
                    Local *local = list_get(currentBlock->locals, i);
                    local->offset += local->size;
                }
                list_add(currentBlock->locals, local);
            } else {
                fprintf(stderr, "%s\n", text);
                for (int32_t i = 0; i < node->token->position; i++) fprintf(stderr, " ");
                fprintf(stderr, "^\nCan't find variable: %s\n", node->token->string);
                exit(EXIT_FAILURE);
            }
        }
    }
}

Node *parser_block(void) {
    Node *node = node_new(NODE_BLOCK);
    node->parentBlock = currentBlock;
    node->statements = list_new(8);
    node->locals = list_new(4);

    if (current()->kind == TOKEN_LCURLY) {
        position++;
        while (current()->kind != TOKEN_RCURLY && current()->kind != TOKEN_EOF) {
            currentBlock = node;
            Node *statement = parser_statement();
            block_create_locals(statement);
            list_add(node->statements, statement);
        }
        parser_eat(TOKEN_RCURLY);
    } else {
        currentBlock = node;
        Node *statement = parser_statement();
        block_create_locals(statement);
        list_add(node->statements, statement);
    }
    return node;
}

Node *parser_statement(void) {
    if (current()->kind == TOKEN_LCURLY) {
        return parser_block();
    }

    if (current()->kind == TOKEN_IF) {
        Node *node = node_new(NODE_IF);
        position++;
        parser_eat(TOKEN_LPAREN);
        node->condition = parser_assign();
        parser_eat(TOKEN_RPAREN);
        node->thenBlock = parser_block();
        if (current()->kind == TOKEN_ELSE) {
            position++;
            node->elseBlock = parser_block();
        } else {
            node->elseBlock = NULL;
        }
        return node;
    }

    if (current()->kind == TOKEN_WHILE) {
        Node *node = node_new(NODE_WHILE);
        position++;
        parser_eat(TOKEN_LPAREN);
        node->condition = parser_assign();
        parser_eat(TOKEN_RPAREN);
        node->thenBlock = parser_block();
        return node;
    }

    if (current()->kind == TOKEN_FOR) {
        Node *node = node_new(NODE_WHILE);
        position++;

        parser_eat(TOKEN_LPAREN);
        if (current()->kind != TOKEN_SEMICOLON) {
            list_add(currentBlock->statements, parser_assign());
        }
        parser_eat(TOKEN_SEMICOLON);
        if (current()->kind != TOKEN_SEMICOLON) {
            node->condition = parser_assign();
        } else {
            node->condition = NULL;
        }
        parser_eat(TOKEN_SEMICOLON);
        Node *increment = NULL;
        if (current()->kind != TOKEN_RPAREN) {
            increment = parser_assign();
        }
        parser_eat(TOKEN_RPAREN);

        node->thenBlock = parser_block();
        if (increment != NULL) {
            list_add(node->thenBlock->statements, increment);
        }
        return node;
    }

    if (current()->kind == TOKEN_RETURN) {
        position++;
        Node *node = node_new_unary(NODE_RETURN, parser_assign());
        parser_eat(TOKEN_SEMICOLON);
        return node;
    }

    if (current()->kind == TOKEN_SEMICOLON) {
        position++;
        return node_new(NODE_NULL);
    }

    Node *node = parser_assign();
    parser_eat(TOKEN_SEMICOLON);
    return node;
}

Node *parser_assign(void) {
    Node *node = parser_logic();
    if (current()->kind == TOKEN_ASSIGN) {
        Node *otherNode = node_new(NODE_ASSIGN);
        otherNode->lhs = node;
        parser_eat(TOKEN_ASSIGN);
        otherNode->rhs = parser_assign();
        otherNode->type = node->type;
        return otherNode;
    }
    return node;
}

Node *parser_logic(void) {
    Node *node = parser_equality();
    while (current()->kind == TOKEN_LOGIC_AND || current()->kind == TOKEN_LOGIC_OR) {
        if (current()->kind == TOKEN_LOGIC_AND) {
            position++;
            node = node_new_operation(NODE_LOGIC_AND, node, parser_equality());
        } else if (current()->kind == TOKEN_LOGIC_OR) {
            position++;
            node = node_new_operation(NODE_LOGIC_OR, node, parser_equality());
        }
    }
    return node;
}

Node *parser_equality(void) {
    Node *node = parser_relational();
    while (current()->kind == TOKEN_EQ || current()->kind == TOKEN_NEQ) {
        if (current()->kind == TOKEN_EQ) {
            position++;
            node = node_new_operation(NODE_EQ, node, parser_relational());
        } else if (current()->kind == TOKEN_NEQ) {
            position++;
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
            position++;
            node = node_new_operation(NODE_LT, node, parser_add());
        } else if (current()->kind == TOKEN_LTEQ) {
            position++;
            node = node_new_operation(NODE_LTEQ, node, parser_add());
        } else if (current()->kind == TOKEN_GT) {
            position++;
            node = node_new_operation(NODE_GT, node, parser_add());
        } else if (current()->kind == TOKEN_GTEQ) {
            position++;
            node = node_new_operation(NODE_GTEQ, node, parser_add());
        }
    }
    return node;
}

Node *parser_add(void) {
    Node *node = parser_mul();
    while (current()->kind == TOKEN_ADD || current()->kind == TOKEN_SUB) {
        if (current()->kind == TOKEN_ADD) {
            position++;
            Node *rhs = parser_mul();
            if (node->type->kind == TYPE_POINTER && rhs->type->kind == TYPE_NUMBER) {
                rhs = node_new_operation(NODE_MUL, rhs, node_new_number(8));
            }
            node = node_new_operation(NODE_ADD, node, rhs);
        }

        else if (current()->kind == TOKEN_SUB) {
            position++;
            Node *rhs = parser_mul();
            if (node->type->kind == TYPE_POINTER && rhs->type->kind == TYPE_NUMBER) {
                rhs = node_new_operation(NODE_MUL, rhs, node_new_number(8));
            }
            node = node_new_operation(NODE_SUB, node, rhs);
            if (node->type->kind == TYPE_POINTER && rhs->type->kind == TYPE_POINTER) {
                node = node_new_operation(NODE_DIV, node, node_new_number(8));
                node->type = type_new(TYPE_NUMBER);
            }
        }
    }
    return node;
}

Node *parser_mul(void) {
    Node *node = parser_unary();
    while (current()->kind == TOKEN_STAR || current()->kind == TOKEN_DIV || current()->kind == TOKEN_MOD) {
        if (current()->kind == TOKEN_STAR) {
            position++;
            node = node_new_operation(NODE_MUL, node, parser_unary());
        } else if (current()->kind == TOKEN_DIV) {
            position++;
            node = node_new_operation(NODE_DIV, node, parser_unary());
        } else if (current()->kind == TOKEN_MOD) {
            position++;
            node = node_new_operation(NODE_MOD, node, parser_unary());
        }
    }
    return node;
}

Node *parser_unary(void) {
    if (current()->kind == TOKEN_ADD) {
        position++;
        return parser_unary();
    }

    if (current()->kind == TOKEN_SUB) {
        position++;
        return node_new_unary(NODE_NEG, parser_unary());
    }

    if (current()->kind == TOKEN_ADDR) {
        position++;
        Node *node = node_new_unary(NODE_ADDR, parser_unary());
        if (node->unary->kind != NODE_VARIABLE) {
            fprintf(stderr, "%s\n", text);
            for (int32_t i = 0; i < ((Token *)list_get(tokens, position - 1))->position; i++) fprintf(stderr, " ");
            fprintf(stderr, "^\nYou can only get an address from a variable\n");
            exit(EXIT_FAILURE);
        }
        node->type = type_pointer(node->unary->type);
        return node;
    }

    if (current()->kind == TOKEN_STAR) {
        position++;
        Node *node = node_new_unary(NODE_DEREF, parser_unary());
        if (node->unary->type->kind != TYPE_POINTER) {
            fprintf(stderr, "%s\n", text);
            for (int32_t i = 0; i < ((Token *)list_get(tokens, position - 1))->position; i++) fprintf(stderr, " ");
            fprintf(stderr, "^\nYou can only dereference a pointer type\n");
            exit(EXIT_FAILURE);
        }
        node->type = node->unary->type->base;
        return node;
    }

    if (current()->kind == TOKEN_LOGIC_NOT) {
        position++;
        return node_new_unary(NODE_LOGIC_NOT, parser_unary());
    }

    return parser_primary();
}

Node *parser_primary(void) {
    if (current()->kind == TOKEN_LPAREN) {
        position++;
        Node *node = parser_logic();
        parser_eat(TOKEN_RPAREN);
        return node;
    }

    if (current()->kind == TOKEN_NUMBER) {
        Node *node = node_new_number(current()->number);
        position++;
        return node;
    }

    if (current()->kind == TOKEN_VARIABLE) {
        Node *node = node_new(NODE_VARIABLE);
        node->type = type_new(TYPE_NUMBER);
        node->string = current()->string;
        node->token = list_get(tokens, position++);
        return node;
    }

    fprintf(stderr, "%s\n", text);
    for (int32_t i = 0; i < current()->position; i++) fprintf(stderr, " ");
    char tokenBuffer[32];
    token_to_string(current()->kind, tokenBuffer);
    fprintf(stderr, "^\nUnexpected: %s\n", tokenBuffer);
    exit(EXIT_FAILURE);
}
