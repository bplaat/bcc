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

void node_dump(FILE *f, Node *node) {
    if (node->type == NODE_INTEGER) {
        fprintf(f, "%lld", node->integer);
    }

    if (node->type > NODE_UNARY_BEGIN && node->type < NODE_UNARY_END) {
        fprintf(f, "( ");

        if (node->type == NODE_NEG) fprintf(f, "- ");

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
        if (node->type == NODE_EQ) fprintf(f, " == ");
        if (node->type == NODE_NEQ) fprintf(f, " != ");
        if (node->type == NODE_LT) fprintf(f, " < ");
        if (node->type == NODE_LTEQ) fprintf(f, " <= ");
        if (node->type == NODE_GT) fprintf(f, " > ");
        if (node->type == NODE_GTEQ) fprintf(f, " >= ");

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
    return parser_equality(&parser);
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

Node *parser_equality(Parser *parser) {
    Node *node = parser_relational(parser);
    while (current()->type == TOKEN_EQ || current()->type == TOKEN_NEQ) {
        if (current()->type == TOKEN_EQ) {
            Token *token = current();
            parser_eat(parser, TOKEN_EQ);
            node = node_new_operation(NODE_EQ, token, node, parser_relational(parser));
        }
        if (current()->type == TOKEN_NEQ) {
            Token *token = current();
            parser_eat(parser, TOKEN_NEQ);
            node = node_new_operation(NODE_NEQ, token, node, parser_relational(parser));
        }
    }
    return node;
}

Node *parser_relational(Parser *parser) {
    Node *node = parser_add(parser);
    while (current()->type == TOKEN_LT || current()->type == TOKEN_LTEQ || current()->type == TOKEN_GT || current()->type == TOKEN_GTEQ) {
        if (current()->type == TOKEN_LT) {
            Token *token = current();
            parser_eat(parser, TOKEN_LT);
            node = node_new_operation(NODE_LT, token, node, parser_add(parser));
        }
        if (current()->type == TOKEN_LTEQ) {
            Token *token = current();
            parser_eat(parser, TOKEN_LTEQ);
            node = node_new_operation(NODE_LTEQ, token, node, parser_add(parser));
        }
        if (current()->type == TOKEN_GT) {
            Token *token = current();
            parser_eat(parser, TOKEN_GT);
            node = node_new_operation(NODE_GT, token, node, parser_add(parser));
        }
        if (current()->type == TOKEN_GTEQ) {
            Token *token = current();
            parser_eat(parser, TOKEN_GTEQ);
            node = node_new_operation(NODE_GTEQ, token, node, parser_add(parser));
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
    return parser_primary(parser);
}


Node *parser_primary(Parser *parser) {
    Token *token = current();

    if (token->type == TOKEN_LPAREN) {
        parser_eat(parser, TOKEN_LPAREN);
        Node *node = parser_equality(parser);
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
