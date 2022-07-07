#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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
Node *currentBlock = NULL;
Token *token;

void parser_eat(TokenType type) {
    if (token->type == type) {
        token++;
    } else {
        fprintf(stderr, "%s\n", text);
        for (int32_t i = 0; i < token->pos; i++) fprintf(stderr, " ");
        char wantedTokenTypeBuffer[32];
        token_type_to_string(type, wantedTokenTypeBuffer);
        char gotTokenTypeBuffer[32];
        token_type_to_string(token->type, gotTokenTypeBuffer);
        fprintf(stderr, "^\nUnexpected: '%s' needed '%s'\n", gotTokenTypeBuffer, wantedTokenTypeBuffer);
        exit(EXIT_FAILURE);
    }
}

void block_create_locals(Node *node) {
    static bool inAssign = false;

    if (node->type == NODE_TYPE_ASSIGN) {
        inAssign = true;
        block_create_locals(node->lhs);
        block_create_locals(node->rhs);
        inAssign = false;
    }

    if (node->type >= NODE_TYPE_NEG && node->type <= NODE_TYPE_LOGIC_NOT) {
        block_create_locals(node->unary);
    }

    if (node->type >= NODE_TYPE_ADD && node->type <= NODE_TYPE_LOGIC_OR) {
        block_create_locals(node->lhs);
        block_create_locals(node->rhs);
    }

    if (node->type == NODE_TYPE_VARIABLE) {
        Local *local = block_find_local(currentBlock, node->string);
        if (local == NULL) {
            if (inAssign) {
                Local *local = malloc(sizeof(Local));
                local->name = node->string;
                local->size = 8;
                local->offset = local->size;
                for (size_t i = 0; i < currentBlock->locals->size; i++) {
                    Local *local = currentBlock->locals->items[i];
                    local->offset += local->size;
                }
                list_add(currentBlock->locals, local);
            } else {
                fprintf(stderr, "%s\n", text);
                for (int32_t i = 0; i < node->token->pos; i++) fprintf(stderr, " ");
                fprintf(stderr, "^\nCan't find variable: %s\n", node->token->string);
                exit(EXIT_FAILURE);
            }
        }
    }
}

Node *parser_block(void) {
    Node *node = malloc(sizeof(Node));
    node->type = NODE_TYPE_BLOCK;
    node->parentBlock = currentBlock;
    node->statements = list_new(8);
    node->locals = list_new(4);

    if (token->type == TOKEN_TYPE_LCURLY) {
        token++;
        while (token->type != TOKEN_TYPE_RCURLY && token->type != TOKEN_TYPE_EOF) {
            currentBlock = node;
            Node *statement = parser_statement();
            block_create_locals(statement);
            list_add(node->statements, statement);
        }
        parser_eat(TOKEN_TYPE_RCURLY);
    } else {
        currentBlock = node;
        Node *statement = parser_statement();
        block_create_locals(statement);
        list_add(node->statements, statement);
    }
    return node;
}

Node *parser_statement(void) {
    if (token->type == TOKEN_TYPE_LCURLY) {
        return parser_block();
    }

    if (token->type == TOKEN_TYPE_IF) {
        Node *node = malloc(sizeof(Node));
        node->type = NODE_TYPE_IF;
        token++;
        parser_eat(TOKEN_TYPE_LPAREN);
        node->condition = parser_assign();
        parser_eat(TOKEN_TYPE_RPAREN);
        node->thenBlock = parser_block();
        if (token->type == TOKEN_TYPE_ELSE) {
            token++;
            node->elseBlock = parser_block();
        } else {
            node->elseBlock = NULL;
        }
        return node;
    }

    if (token->type == TOKEN_TYPE_WHILE) {
        Node *node = malloc(sizeof(Node));
        node->type = NODE_TYPE_WHILE;
        token++;
        parser_eat(TOKEN_TYPE_LPAREN);
        node->condition = parser_assign();
        parser_eat(TOKEN_TYPE_RPAREN);
        node->thenBlock = parser_block();
        return node;
    }

    if (token->type == TOKEN_TYPE_FOR) {
        Node *node = malloc(sizeof(Node));
        node->type = NODE_TYPE_WHILE;
        token++;

        parser_eat(TOKEN_TYPE_LPAREN);
        if (token->type != TOKEN_TYPE_SEMICOLON) {
            list_add(currentBlock->statements, parser_assign());
        }
        parser_eat(TOKEN_TYPE_SEMICOLON);
        if (token->type != TOKEN_TYPE_SEMICOLON) {
            node->condition = parser_assign();
        } else {
            node->condition = NULL;
        }
        parser_eat(TOKEN_TYPE_SEMICOLON);
        Node *increment = NULL;
        if (token->type != TOKEN_TYPE_RPAREN) {
            increment = parser_assign();
        }
        parser_eat(TOKEN_TYPE_RPAREN);

        node->thenBlock = parser_block();
        if (increment != NULL) {
            list_add(node->thenBlock->statements, increment);
        }
        return node;
    }

    if (token->type == TOKEN_TYPE_RETURN) {
        Node *node = malloc(sizeof(Node));
        node->type = NODE_TYPE_RETURN;
        token++;

        node->unary = parser_assign();
        parser_eat(TOKEN_TYPE_SEMICOLON);
        return node;
    }

    if (token->type == TOKEN_TYPE_SEMICOLON) {
        Node *node = malloc(sizeof(Node));
        node->type = NODE_TYPE_NULL;
        token++;
        return node;
    }

    Node *node = parser_assign();
    parser_eat(TOKEN_TYPE_SEMICOLON);
    return node;
}

Node *parser_assign(void) {
    Node *node = parser_logic();
    if (token->type == TOKEN_TYPE_ASSIGN) {
        Node *otherNode = malloc(sizeof(Node));
        otherNode->type = NODE_TYPE_ASSIGN;
        otherNode->lhs = node;
        parser_eat(TOKEN_TYPE_ASSIGN);
        otherNode->rhs = parser_assign();
        return otherNode;
    }
    return node;
}

Node *parser_logic(void) {
    Node *node = parser_equality();
    while (token->type == TOKEN_TYPE_LOGIC_AND || token->type == TOKEN_TYPE_LOGIC_OR) {
        Node *otherNode = malloc(sizeof(Node));
        if (token->type == TOKEN_TYPE_LOGIC_AND) otherNode->type = NODE_TYPE_LOGIC_AND;
        if (token->type == TOKEN_TYPE_LOGIC_OR) otherNode->type = NODE_TYPE_LOGIC_OR;
        token++;
        otherNode->lhs = node;
        otherNode->rhs = parser_equality();
        node = otherNode;
    }
    return node;
}

Node *parser_equality(void) {
    Node *node = parser_relational();
    while (token->type == TOKEN_TYPE_EQ || token->type == TOKEN_TYPE_NEQ) {
        Node *otherNode = malloc(sizeof(Node));
        if (token->type == TOKEN_TYPE_EQ) otherNode->type = NODE_TYPE_EQ;
        if (token->type == TOKEN_TYPE_NEQ) otherNode->type = NODE_TYPE_NEQ;
        token++;
        otherNode->lhs = node;
        otherNode->rhs = parser_relational();
        node = otherNode;
    }
    return node;
}

Node *parser_relational(void) {
    Node *node = parser_add();
    while (token->type == TOKEN_TYPE_LT || token->type == TOKEN_TYPE_LTEQ || token->type == TOKEN_TYPE_GT || token->type == TOKEN_TYPE_GTEQ) {
        Node *otherNode = malloc(sizeof(Node));
        if (token->type == TOKEN_TYPE_LT) otherNode->type = NODE_TYPE_LT;
        if (token->type == TOKEN_TYPE_LTEQ) otherNode->type = NODE_TYPE_LTEQ;
        if (token->type == TOKEN_TYPE_GT) otherNode->type = NODE_TYPE_GT;
        if (token->type == TOKEN_TYPE_GTEQ) otherNode->type = NODE_TYPE_GTEQ;
        token++;
        otherNode->lhs = node;
        otherNode->rhs = parser_add();
        node = otherNode;
    }
    return node;
}

Node *parser_add(void) {
    Node *node = parser_mul();
    while (token->type == TOKEN_TYPE_ADD || token->type == TOKEN_TYPE_SUB) {
        Node *otherNode = malloc(sizeof(Node));
        if (token->type == TOKEN_TYPE_ADD) otherNode->type = NODE_TYPE_ADD;
        if (token->type == TOKEN_TYPE_SUB) otherNode->type = NODE_TYPE_SUB;
        token++;
        otherNode->lhs = node;
        otherNode->rhs = parser_mul();
        node = otherNode;
    }
    return node;
}

Node *parser_mul(void) {
    Node *node = parser_unary();
    while (token->type == TOKEN_TYPE_STAR || token->type == TOKEN_TYPE_DIV || token->type == TOKEN_TYPE_MOD) {
        Node *otherNode = malloc(sizeof(Node));
        if (token->type == TOKEN_TYPE_STAR) otherNode->type = NODE_TYPE_MUL;
        if (token->type == TOKEN_TYPE_DIV) otherNode->type = NODE_TYPE_DIV;
        if (token->type == TOKEN_TYPE_MOD) otherNode->type = NODE_TYPE_MOD;
        token++;
        otherNode->lhs = node;
        otherNode->rhs = parser_unary();
        node = otherNode;
    }
    return node;
}

Node *parser_unary(void) {
    if (token->type == TOKEN_TYPE_ADD) {
        token++;
        return parser_unary();
    }

    if (token->type == TOKEN_TYPE_SUB) {
        token++;
        Node *node = malloc(sizeof(Node));
        node->type = NODE_TYPE_NEG;
        node->unary = parser_unary();
        return node;
    }

    if (token->type == TOKEN_TYPE_ADDR) {
        Node *node = malloc(sizeof(Node));
        node->type = NODE_TYPE_ADDR;
        token++;
        node->unary = parser_unary();
        if (node->unary->type != NODE_TYPE_VARIABLE) {
            fprintf(stderr, "%s\n", text);
            for (int32_t i = 0; i < (token - 1)->pos; i++) fprintf(stderr, " ");
            fprintf(stderr, "^\nYou can only get an address from a variable\n");
            exit(EXIT_FAILURE);
        }
        return node;
    }

    if (token->type == TOKEN_TYPE_STAR) {
        token++;
        Node *node = malloc(sizeof(Node));
        node->type = NODE_TYPE_DEREF;
        node->unary = parser_unary();
        return node;
    }

    if (token->type == TOKEN_TYPE_LOGIC_NOT) {
        token++;
        Node *node = malloc(sizeof(Node));
        node->type = NODE_TYPE_LOGIC_NOT;
        node->unary = parser_unary();
        return node;
    }

    return parser_primary();
}

Node *parser_primary(void) {
    if (token->type == TOKEN_TYPE_LPAREN) {
        token++;
        Node *node = parser_logic();
        parser_eat(TOKEN_TYPE_RPAREN);
        return node;
    }

    if (token->type == TOKEN_TYPE_NUMBER) {
        Node *node = malloc(sizeof(Node));
        node->type = NODE_TYPE_NUMBER;
        node->number = token->number;
        token++;
        return node;
    }

    if (token->type == TOKEN_TYPE_VARIABLE) {
        Node *node = malloc(sizeof(Node));
        node->type = NODE_TYPE_VARIABLE;
        node->string = token->string;
        node->token = token++;
        return node;
    }

    fprintf(stderr, "%s\n", text);
    for (int32_t i = 0; i < token->pos; i++) fprintf(stderr, " ");
    char tokenTypeBuffer[32];
    token_type_to_string(token->type, tokenTypeBuffer);
    fprintf(stderr, "^\nUnexpected: %s\n", tokenTypeBuffer);
    exit(EXIT_FAILURE);
}

Local *block_find_local(Node *block, char *name) {
    if (block->parentBlock != NULL) {
        Local *local = block_find_local(block->parentBlock, name);
        if (local != NULL) {
            return local;
        }
    }

    for (size_t i = 0; i < block->locals->size; i++) {
        Local *local = block->locals->items[i];
        if (!strcmp(local->name, name)) {
            return local;
        }
    }
    return NULL;
}

void node_print(FILE *file, Node *node) {
    if (node->type == NODE_TYPE_BLOCK) {
        fprintf(file, "{ ");
        for (size_t i = 0; i < node->locals->size; i++) {
            Local *local = node->locals->items[i];
            fprintf(file, "int %s; ", local->name);
        }
        for (size_t i = 0; i < node->statements->size; i++) {
            node_print(file, node->statements->items[i]);
            fprintf(file, "; ");
        }
        fprintf(file, " }");
        return;
    }

    if (node->type == NODE_TYPE_NUMBER) {
        fprintf(file, "%lld", node->number);
    }
    if (node->type == NODE_TYPE_VARIABLE) {
        fprintf(file, "%s", node->string);
    }

    if (node->type == NODE_TYPE_IF) {
        fprintf(file, "if (");
        node_print(file, node->condition);
        fprintf(file, ") ");
        node_print(file, node->thenBlock);
        if (node->elseBlock != NULL) {
            fprintf(file, " else ");
            node_print(file, node->elseBlock);
        }
    }

    if (node->type == NODE_TYPE_WHILE) {
        fprintf(file, "while (");
        if (node->condition != NULL) {
            node_print(file, node->condition);
        } else {
            fprintf(file, "1");
        }
        fprintf(file, ") ");
        node_print(file, node->thenBlock);
    }

    if (node->type == NODE_TYPE_RETURN) {
        fprintf(file, "return ");
        node_print(file, node->unary);
    }

    if (node->type >= NODE_TYPE_NEG && node->type <= NODE_TYPE_LOGIC_NOT) {
        if (node->type == NODE_TYPE_NEG) fprintf(file, "(- ");
        if (node->type == NODE_TYPE_ADDR) fprintf(file, "(& ");
        if (node->type == NODE_TYPE_DEREF) fprintf(file, "(* ");
        if (node->type == NODE_TYPE_LOGIC_NOT) fprintf(file, "(! ");
        node_print(file, node->unary);
        fprintf(file, ")");
    }

    if (node->type >= NODE_TYPE_ASSIGN && node->type <= NODE_TYPE_LOGIC_OR) {
        fprintf(file, "(");
        node_print(file, node->lhs);
        if (node->type == NODE_TYPE_ASSIGN) fprintf(file, " = ");
        if (node->type == NODE_TYPE_ADD) fprintf(file, " + ");
        if (node->type == NODE_TYPE_SUB) fprintf(file, " - ");
        if (node->type == NODE_TYPE_MUL) fprintf(file, " * ");
        if (node->type == NODE_TYPE_DIV) fprintf(file, " / ");
        if (node->type == NODE_TYPE_MOD) fprintf(file, " %% ");
        if (node->type == NODE_TYPE_EQ) fprintf(file, " == ");
        if (node->type == NODE_TYPE_NEQ) fprintf(file, " != ");
        if (node->type == NODE_TYPE_LT) fprintf(file, " < ");
        if (node->type == NODE_TYPE_LTEQ) fprintf(file, " <= ");
        if (node->type == NODE_TYPE_GT) fprintf(file, " > ");
        if (node->type == NODE_TYPE_GTEQ) fprintf(file, " >= ");
        if (node->type == NODE_TYPE_LOGIC_AND) fprintf(file, " && ");
        if (node->type == NODE_TYPE_LOGIC_OR) fprintf(file, " || ");
        node_print(file, node->rhs);
        fprintf(file, ")");
    }
}
