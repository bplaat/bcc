#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

// Node
Node *node_new(NodeType type) {
    Node *node = malloc(sizeof(Node));
    node->type = type;
    return node;
}

Node *node_new_operation(NodeType type, Node *lhs, Node *rhs) {
    Node *node = node_new(type);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

void node_dump(FILE *f, Node *node) {
    if (node->type == NODE_INTEGER) {
        fprintf(f, "%lld", node->integer);
    }

    if (node->type > NODE_OPERATION_BEGIN && node->type < NODE_OPERATION_END) {
        fprintf(f, "( ");
        node_dump(f, node->lhs);

        if (node->type == NODE_ADD) fprintf(f, " + ");
        if (node->type == NODE_SUB) fprintf(f, " - ");

        node_dump(f, node->rhs);
        fprintf(f, " )");
    }
}

// Parser
Node *parser_add(Lexer *lexer) {
    Node *node = parser_primary(lexer);
    for (;;) {
        Token token = lexer_next_token(lexer);
        if (token.type == TOKEN_ADD) {
            node = node_new_operation(NODE_ADD, node, parser_primary(lexer));
        } else if (token.type == TOKEN_SUB) {
            node = node_new_operation(NODE_SUB, node, parser_primary(lexer));;
        } else {
            break;
        }
    }
    return node;
}

Node *parser_primary(Lexer *lexer) {
    Token token = lexer_next_token(lexer);

    if (token.type == TOKEN_INTEGER) {
        Node *node = node_new(NODE_INTEGER);
        node->integer = token.integer;
        return node;
    }

    fprintf(stderr, "Unexpected token: %d\n", token.type);
    exit(EXIT_FAILURE);
}
