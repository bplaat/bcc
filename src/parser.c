#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

// Node
Node *node_new(NodeType type, Token *token) {
    Node *node = malloc(sizeof(Node));
    node->type = type;
    node->token = token;
    return node;
}

Node *node_new_unary(NodeType type, Token *token, Node *unary) {
    Node *node = node_new(type, token);
    node->unary = unary;
    return node;
}

Node *node_new_operation(NodeType type, Token *token, Node *lhs, Node *rhs) {
    Node *node = node_new(type, token);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *node_new_nodes(NodeType type, Token *token) {
    Node *node = node_new(type, token);
    node->nodes.capacity = 8;
    list_init(&node->nodes);
    return node;
}

void node_dump(FILE *f, Node *node) {
    if (node->type == NODE_BLOCK) {
        for (size_t i = 0; i < node->nodes.size; i++) {
            Node *child = node->nodes.items[i];
            node_dump(f, child);
            fprintf(f, ";\n");
        }
    }

    if (node->type == NODE_INTEGER) {
        fprintf(f, "%lld", node->integer);
    }

    if (node->type > NODE_UNARY_BEGIN && node->type < NODE_UNARY_END) {
        fprintf(f, "( ");

        if (node->type == NODE_NEG) fprintf(f, "- ");
        if (node->type == NODE_NOT) fprintf(f, "~ ");
        if (node->type == NODE_LOGICAL_NOT) fprintf(f, "! ");

        node_dump(f, node->unary);
        fprintf(f, " )");
    }

    if (node->type > NODE_OPERATION_BEGIN && node->type < NODE_OPERATION_END) {
        fprintf(f, "( ");
        node_dump(f, node->lhs);

        if (node->type == NODE_ADD) fprintf(f, " + ");
        if (node->type == NODE_SUB) fprintf(f, " - ");
        if (node->type == NODE_MUL) fprintf(f, " * ");
        if (node->type == NODE_DIV) fprintf(f, " / ");
        if (node->type == NODE_MOD) fprintf(f, " %% ");
        if (node->type == NODE_AND) fprintf(f, " & ");
        if (node->type == NODE_OR) fprintf(f, " | ");
        if (node->type == NODE_XOR) fprintf(f, " ^ ");
        if (node->type == NODE_SHL) fprintf(f, " << ");
        if (node->type == NODE_SHR) fprintf(f, " >> ");
        if (node->type == NODE_EQ) fprintf(f, " == ");
        if (node->type == NODE_NEQ) fprintf(f, " != ");
        if (node->type == NODE_LT) fprintf(f, " < ");
        if (node->type == NODE_LTEQ) fprintf(f, " <= ");
        if (node->type == NODE_GT) fprintf(f, " > ");
        if (node->type == NODE_GTEQ) fprintf(f, " >= ");
        if (node->type == NODE_LOGICAL_AND) fprintf(f, " && ");
        if (node->type == NODE_LOGICAL_OR) fprintf(f, " || ");

        node_dump(f, node->rhs);
        fprintf(f, " )");
    }
}

// Parser
Node *parser(Token *tokens, size_t tokens_size) {
    Parser parser = {
        .tokens = tokens,
        .tokens_size = tokens_size,
        .position = 0,
    };
    return parser_block(&parser);
}

#define current() (&parser->tokens[parser->position])

void parser_eat(Parser *parser, TokenType type) {
    Token *token = current();
    if (token->type == type) {
        parser->position++;
    } else {
        fprintf(stderr, "Unexpected token: %d at %d:%d\n", token->type, token->line, token->column);
        exit(EXIT_FAILURE);
    }
}

Node *parser_block(Parser *parser) {
    Token *token = current();
    Node *node = node_new_nodes(NODE_BLOCK, token);
    if (token->type == TOKEN_LCURLY) {
        parser_eat(parser, TOKEN_LCURLY);
        while (current()->type != TOKEN_RCURLY) {
            Node *child = parser_statement(parser);
            if (child != NULL) list_add(&node->nodes, child);
        }
        parser_eat(parser, TOKEN_RCURLY);
    } else {
        Node *child = parser_statement(parser);
        if (child != NULL) list_add(&node->nodes, child);
    }
    return node;
}

Node *parser_statement(Parser *parser) {
    Token *token = current();

    if (token->type == TOKEN_SEMICOLON) {
        parser_eat(parser, TOKEN_SEMICOLON);
        return NULL;
    }
    if (token->type == TOKEN_LCURLY) {
        return parser_block(parser);
    }

    Node *node = parser_logical(parser);
    parser_eat(parser, TOKEN_SEMICOLON);
    return node;
}

Node *parser_logical(Parser *parser) {
    Node *node = parser_equality(parser);
    while (current()->type == TOKEN_LOGICAL_AND || current()->type == TOKEN_LOGICAL_OR) {
        if (current()->type == TOKEN_LOGICAL_AND) {
            Token *token = current();
            parser_eat(parser, TOKEN_LOGICAL_AND);
            node = node_new_operation(NODE_LOGICAL_AND, token, node, parser_equality(parser));
        }
        if (current()->type == TOKEN_LOGICAL_OR) {
            Token *token = current();
            parser_eat(parser, TOKEN_LOGICAL_OR);
            node = node_new_operation(NODE_LOGICAL_OR, token, node, parser_equality(parser));
        }
    }
    return node;
}

Node *parser_equality(Parser *parser) {
    Node *node = parser_relational(parser);
    while (current()->type == TOKEN_EQ || current()->type == TOKEN_NEQ) {
        Token *token = current();
        if (token->type == TOKEN_EQ) {
            parser_eat(parser, TOKEN_EQ);
            node = node_new_operation(NODE_EQ, token, node, parser_relational(parser));
        }
        if (token->type == TOKEN_NEQ) {
            parser_eat(parser, TOKEN_NEQ);
            node = node_new_operation(NODE_NEQ, token, node, parser_relational(parser));
        }
    }
    return node;
}

Node *parser_relational(Parser *parser) {
    Node *node = parser_bitwise(parser);
    while (current()->type == TOKEN_LT || current()->type == TOKEN_LTEQ || current()->type == TOKEN_GT ||
           current()->type == TOKEN_GTEQ) {
        Token *token = current();
        if (token->type == TOKEN_LT) {
            parser_eat(parser, TOKEN_LT);
            node = node_new_operation(NODE_LT, token, node, parser_bitwise(parser));
        }
        if (token->type == TOKEN_LTEQ) {
            parser_eat(parser, TOKEN_LTEQ);
            node = node_new_operation(NODE_LTEQ, token, node, parser_bitwise(parser));
        }
        if (token->type == TOKEN_GT) {
            parser_eat(parser, TOKEN_GT);
            node = node_new_operation(NODE_GT, token, node, parser_bitwise(parser));
        }
        if (token->type == TOKEN_GTEQ) {
            parser_eat(parser, TOKEN_GTEQ);
            node = node_new_operation(NODE_GTEQ, token, node, parser_bitwise(parser));
        }
    }
    return node;
}

Node *parser_bitwise(Parser *parser) {
    Node *node = parser_shift(parser);
    while (current()->type == TOKEN_AND || current()->type == TOKEN_XOR || current()->type == TOKEN_OR) {
        Token *token = current();
        if (token->type == TOKEN_AND) {
            parser_eat(parser, TOKEN_AND);
            node = node_new_operation(NODE_AND, token, node, parser_shift(parser));
        }
        if (token->type == TOKEN_XOR) {
            parser_eat(parser, TOKEN_XOR);
            node = node_new_operation(NODE_XOR, token, node, parser_shift(parser));
        }
        if (token->type == TOKEN_OR) {
            parser_eat(parser, TOKEN_OR);
            node = node_new_operation(NODE_OR, token, node, parser_shift(parser));
        }
    }
    return node;
}

Node *parser_shift(Parser *parser) {
    Node *node = parser_add(parser);
    while (current()->type == TOKEN_SHL || current()->type == TOKEN_SHR) {
        Token *token = current();
        if (token->type == TOKEN_SHL) {
            parser_eat(parser, TOKEN_SHL);
            node = node_new_operation(NODE_SHL, token, node, parser_add(parser));
        }
        if (token->type == TOKEN_SHR) {
            parser_eat(parser, TOKEN_SHR);
            node = node_new_operation(NODE_SHR, token, node, parser_add(parser));
        }
    }
    return node;
}

Node *parser_add(Parser *parser) {
    Node *node = parser_mul(parser);
    while (current()->type == TOKEN_ADD || current()->type == TOKEN_SUB) {
        Token *token = current();
        if (token->type == TOKEN_ADD) {
            parser_eat(parser, TOKEN_ADD);
            node = node_new_operation(NODE_ADD, token, node, parser_mul(parser));
        }
        if (token->type == TOKEN_SUB) {
            parser_eat(parser, TOKEN_SUB);
            node = node_new_operation(NODE_SUB, token, node, parser_mul(parser));
        }
    }
    return node;
}

Node *parser_mul(Parser *parser) {
    Node *node = parser_unary(parser);
    while (current()->type == TOKEN_MUL || current()->type == TOKEN_DIV || current()->type == TOKEN_MOD) {
        Token *token = current();
        if (token->type == TOKEN_MUL) {
            parser_eat(parser, TOKEN_MUL);
            node = node_new_operation(NODE_MUL, token, node, parser_unary(parser));
        }
        if (token->type == TOKEN_DIV) {
            parser_eat(parser, TOKEN_DIV);
            node = node_new_operation(NODE_DIV, token, node, parser_unary(parser));
        }
        if (token->type == TOKEN_MOD) {
            parser_eat(parser, TOKEN_MOD);
            node = node_new_operation(NODE_MOD, token, node, parser_unary(parser));
        }
    }
    return node;
}

Node *parser_unary(Parser *parser) {
    Token *token = current();
    if (token->type == TOKEN_ADD) {
        parser_eat(parser, TOKEN_ADD);
        return parser_unary(parser);
    }
    if (token->type == TOKEN_SUB) {
        parser_eat(parser, TOKEN_SUB);
        return node_new_unary(NODE_NEG, token, parser_unary(parser));
    }
    if (token->type == TOKEN_NOT) {
        parser_eat(parser, TOKEN_NOT);
        return node_new_unary(NODE_NOT, token, parser_unary(parser));
    }
    if (token->type == TOKEN_LOGICAL_NOT) {
        parser_eat(parser, TOKEN_LOGICAL_NOT);
        return node_new_unary(NODE_LOGICAL_NOT, token, parser_unary(parser));
    }
    return parser_primary(parser);
}

Node *parser_primary(Parser *parser) {
    Token *token = current();

    if (token->type == TOKEN_LPAREN) {
        parser_eat(parser, TOKEN_LPAREN);
        Node *node = parser_logical(parser);
        parser_eat(parser, TOKEN_RPAREN);
        return node;
    }
    if (token->type == TOKEN_INTEGER) {
        Node *node = node_new(NODE_INTEGER, token);
        node->integer = token->integer;
        parser_eat(parser, TOKEN_INTEGER);
        return node;
    }

    fprintf(stderr, "Unexpected token: %d at %d:%d\n", token->type, token->line, token->column);
    exit(EXIT_FAILURE);
}
