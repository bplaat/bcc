#include "parser.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

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

Node *node_new_function(NodeType type, Token *token) {
    Node *node = node_new_nodes(type, token);
    node->locals.capacity = 8;
    map_init(&node->locals);
    node->locals_size = 0;
    return node;
}

void node_dump(FILE *f, Node *node, int32_t indent) {
    if (node->type == NODE_FUNCTION) {
        fprintf(f, "{\n");
        for (size_t i = 0; i < node->locals.capacity; i++) {
            char *key = node->locals.keys[i];
            if (key) {
                Local *local = node->locals.values[i];
                for (int32_t i = 0; i < indent + 1; i++) fprintf(f, "  ");
                fprintf(f, "long %s;\n", local->name);
            }
        }
        for (size_t i = 0; i < node->nodes.size; i++) {
            Node *child = node->nodes.items[i];
            for (int32_t i = 0; i < indent + 1; i++) fprintf(f, "  ");
            node_dump(f, child, indent + 1);
            fprintf(f, ";\n");
        }
        fprintf(f, "}\n");
    }
    if (node->type == NODE_NODES) {
        fprintf(f, "{\n");
        for (size_t i = 0; i < node->nodes.size; i++) {
            Node *child = node->nodes.items[i];
            for (int32_t i = 0; i < indent + 1; i++) fprintf(f, "  ");
            node_dump(f, child, indent + 1);
            fprintf(f, ";\n");
        }

        for (int32_t i = 0; i < indent; i++) fprintf(f, "  ");
        fprintf(f, "}");
    }

    if (node->type == NODE_LOCAL) {
        fprintf(f, "%s", node->local->name);
    }
    if (node->type == NODE_INTEGER) {
        fprintf(f, "%" PRIi64, node->integer);
    }

    if (node->type == NODE_IF) {
        fprintf(f, "if (");
        node_dump(f, node->condition, indent);
        fprintf(f, ") ");
        node_dump(f, node->thenBlock, indent + 1);
        if (node->elseBlock != NULL) {
            for (int32_t i = 0; i < indent; i++) fprintf(f, "  ");
            fprintf(f, " else ");
            node_dump(f, node->elseBlock, indent + 1);
        }
    }
    if (node->type == NODE_WHILE) {
        fprintf(f, "while (");
        node_dump(f, node->condition, indent);
        fprintf(f, ") ");
        node_dump(f, node->thenBlock, indent + 1);
    }
    if (node->type == NODE_DOWHILE) {
        fprintf(f, "do ");
        node_dump(f, node->thenBlock, indent + 1);
        for (int32_t i = 0; i < indent; i++) fprintf(f, "  ");
        fprintf(f, "while (");
        node_dump(f, node->condition, indent);
        fprintf(f, ")");
    }
    if (node->type == NODE_RETURN) {
        fprintf(f, "return ");
        node_dump(f, node->unary, indent);
    }

    if (node->type > NODE_UNARY_BEGIN && node->type < NODE_UNARY_END) {
        fprintf(f, "( ");

        if (node->type == NODE_NEG) fprintf(f, "- ");
        if (node->type == NODE_NOT) fprintf(f, "~ ");
        if (node->type == NODE_LOGICAL_NOT) fprintf(f, "! ");

        node_dump(f, node->unary, indent);
        fprintf(f, " )");
    }

    if (node->type > NODE_OPERATION_BEGIN && node->type < NODE_OPERATION_END) {
        fprintf(f, "( ");
        node_dump(f, node->lhs, indent);

        if (node->type == NODE_ASSIGN) fprintf(f, " = ");
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

        node_dump(f, node->rhs, indent);
        fprintf(f, " )");
    }
}

// Print error
void print_error(char *text, Token *token, char *fmt, ...) {
    fprintf(stderr, "stdin:%d:%d ERROR: ", token->line, token->column);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    // Seek to the right line in text
    char *c = text;
    for (int32_t i = 0; i < token->line - 1; i++) {
        while (*c != '\n' && *c != '\r') c++;
        if (*c == '\r') c++;
        c++;
    }
    char *line_start = c;
    while (*c != '\n' && *c != '\r' && *c != '\0') c++;
    int32_t line_length = c - line_start;

    fprintf(stderr, "\n%4d | ", token->line);
    fwrite(line_start, 1, line_length, stderr);
    fprintf(stderr, "\n     | ");
    for (int32_t i = 0; i < token->column - 1; i++) fprintf(stderr, " ");
    fprintf(stderr, "^\n");
}

// Parser
Node *parser(char *text, Token *tokens, size_t tokens_size) {
    Parser parser = {
        .text = text,
        .tokens = tokens,
        .tokens_size = tokens_size,
        .position = 0,
    };
    return parser_function(&parser);
}

#define current() (&parser->tokens[parser->position])

void parser_eat(Parser *parser, TokenType type) {
    Token *token = current();
    if (token->type == type) {
        parser->position++;
    } else {
        print_error(parser->text, token, "Unexpected token: '%s' wanted '%s'", token_type_to_string(token->type), token_type_to_string(type));
        exit(EXIT_FAILURE);
    }
}

Node *parser_function(Parser *parser) {
    Token *token = current();
    Node *node = node_new_function(NODE_FUNCTION, token);
    Node *oldFunction = parser->currentFunction;
    parser->currentFunction = node;

    parser_eat(parser, TOKEN_LCURLY);
    while (current()->type != TOKEN_RCURLY) {
        Node *child = parser_statement(parser);
        if (child != NULL) list_add(&node->nodes, child);
    }
    parser_eat(parser, TOKEN_RCURLY);

    parser->currentFunction = oldFunction;
    return node;
}

Node *parser_block(Parser *parser) {
    Token *token = current();
    Node *node = node_new_nodes(NODE_NODES, token);
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

    if (token->type == TOKEN_IF) {
        Node *node = node_new(NODE_IF, token);
        parser_eat(parser, TOKEN_IF);
        parser_eat(parser, TOKEN_LPAREN);
        node->condition = parser_assign(parser);
        parser_eat(parser, TOKEN_RPAREN);
        node->thenBlock = parser_block(parser);
        if (current()->type == TOKEN_ELSE) {
            parser_eat(parser, TOKEN_ELSE);
            node->elseBlock = parser_block(parser);
        } else {
            node->elseBlock = NULL;
        }
        return node;
    }

    if (token->type == TOKEN_WHILE) {
        Node *node = node_new(NODE_WHILE, token);
        node->elseBlock = NULL;
        parser_eat(parser, TOKEN_WHILE);
        parser_eat(parser, TOKEN_LPAREN);
        node->condition = parser_assigns(parser);
        parser_eat(parser, TOKEN_RPAREN);
        node->thenBlock = parser_block(parser);
        return node;
    }

    if (token->type == TOKEN_DO) {
        Node *node = node_new(NODE_DOWHILE, token);
        node->elseBlock = NULL;
        parser_eat(parser, TOKEN_DO);
        node->thenBlock = parser_block(parser);
        parser_eat(parser, TOKEN_WHILE);
        parser_eat(parser, TOKEN_LPAREN);
        node->condition = parser_assigns(parser);
        parser_eat(parser, TOKEN_RPAREN);
        parser_eat(parser, TOKEN_SEMICOLON);
        return node;
    }

    if (token->type == TOKEN_FOR) {
        Node *parentNode = node_new_nodes(NODE_NODES, token);
        Node *node = node_new(NODE_WHILE, token);
        node->thenBlock = node_new_nodes(NODE_NODES, token);

        parser_eat(parser, TOKEN_FOR);
        parser_eat(parser, TOKEN_LPAREN);

        if (current()->type != TOKEN_SEMICOLON) {
            list_add(&parentNode->nodes, parser_assigns(parser));
        }
        parser_eat(parser, TOKEN_SEMICOLON);

        list_add(&parentNode->nodes, node);

        if (current()->type != TOKEN_SEMICOLON) {
            node->condition = parser_assign(parser);
        } else {
            node->condition = NULL;
        }
        parser_eat(parser, TOKEN_SEMICOLON);

        Node *incrementNode = NULL;
        if (current()->type != TOKEN_RPAREN) {
            incrementNode = parser_assigns(parser);
        }
        parser_eat(parser, TOKEN_RPAREN);

        list_add(&node->thenBlock->nodes, parser_block(parser));
        if (incrementNode != NULL) {
            list_add(&node->thenBlock->nodes, incrementNode);
        }
        return parentNode;
    }

    if (token->type == TOKEN_RETURN) {
        parser_eat(parser, TOKEN_RETURN);
        Node *node = node_new_unary(NODE_RETURN, token, parser_assign(parser));
        parser_eat(parser, TOKEN_SEMICOLON);
        return node;
    }

    Node *node = parser_assigns(parser);
    parser_eat(parser, TOKEN_SEMICOLON);
    return node;
}

Node *parser_assigns(Parser *parser) {
    Token *token = current();
    List *nodes = list_new();
    for (;;) {
        list_add(nodes, parser_assign(parser));
        if (current()->type == TOKEN_COMMA) {
            parser_eat(parser, TOKEN_COMMA);
        } else {
            break;
        }
    }
    if (nodes->size == 1) {
        Node *first = list_get(nodes, 0);
        list_free(nodes, NULL);
        return first;
    }
    Node *node = node_new_nodes(NODE_NODES, token);
    list_free(&node->nodes, NULL);
    node->nodes = *nodes;
    return node;
}

Node *parser_assign(Parser *parser) {
    Node *lhs = parser_logical(parser);
    if (current()->type > TOKEN_ASSIGN_BEGIN && current()->type < TOKEN_ASSIGN_END) {
        Token *token = current();
        if (lhs->type != NODE_LOCAL) {
            print_error(parser->text, lhs->token, "Unexpected token: '%s' wanted '%s'", token_type_to_string(lhs->token->type),
                        token_type_to_string(TOKEN_VARIABLE));
            exit(EXIT_FAILURE);
        }

        if (token->type == TOKEN_ASSIGN) {
            parser_eat(parser, TOKEN_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, parser_assign(parser));
        }
        if (token->type == TOKEN_ADD_ASSIGN) {
            parser_eat(parser, TOKEN_ADD_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, node_new_operation(NODE_ADD, token, lhs, parser_assign(parser)));
        }
        if (token->type == TOKEN_SUB_ASSIGN) {
            parser_eat(parser, TOKEN_SUB_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, node_new_operation(NODE_SUB, token, lhs, parser_assign(parser)));
        }
        if (token->type == TOKEN_MUL_ASSIGN) {
            parser_eat(parser, TOKEN_MUL_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, node_new_operation(NODE_MUL, token, lhs, parser_assign(parser)));
        }
        if (token->type == TOKEN_DIV_ASSIGN) {
            parser_eat(parser, TOKEN_DIV_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, node_new_operation(NODE_DIV, token, lhs, parser_assign(parser)));
        }
        if (token->type == TOKEN_MOD_ASSIGN) {
            parser_eat(parser, TOKEN_MOD_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, node_new_operation(NODE_MOD, token, lhs, parser_assign(parser)));
        }
        if (token->type == TOKEN_AND_ASSIGN) {
            parser_eat(parser, TOKEN_AND_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, node_new_operation(NODE_AND, token, lhs, parser_assign(parser)));
        }
        if (token->type == TOKEN_OR_ASSIGN) {
            parser_eat(parser, TOKEN_OR_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, node_new_operation(NODE_OR, token, lhs, parser_assign(parser)));
        }
        if (token->type == TOKEN_XOR_ASSIGN) {
            parser_eat(parser, TOKEN_XOR_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, node_new_operation(NODE_XOR, token, lhs, parser_assign(parser)));
        }
        if (token->type == TOKEN_SHL_ASSIGN) {
            parser_eat(parser, TOKEN_SHL_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, node_new_operation(NODE_SHL, token, lhs, parser_assign(parser)));
        }
        if (token->type == TOKEN_SHR_ASSIGN) {
            parser_eat(parser, TOKEN_SHR_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, node_new_operation(NODE_SHR, token, lhs, parser_assign(parser)));
        }
    }
    return lhs;
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
    while (current()->type == TOKEN_LT || current()->type == TOKEN_LTEQ || current()->type == TOKEN_GT || current()->type == TOKEN_GTEQ) {
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
        Node *node = parser_assign(parser);
        parser_eat(parser, TOKEN_RPAREN);
        return node;
    }
    if (token->type == TOKEN_INTEGER) {
        Node *node = node_new(NODE_INTEGER, token);
        node->integer = token->integer;
        parser_eat(parser, TOKEN_INTEGER);
        return node;
    }
    if (token->type == TOKEN_VARIABLE) {
        Node *node = node_new(NODE_LOCAL, token);

        // Create local when it doesn't exists
        Map *locals = &parser->currentFunction->locals;
        Local *local = map_get(locals, token->variable);
        if (local == NULL) {
            local = malloc(sizeof(Local));
            local->name = token->variable;
            local->size = 8;
            local->address = local->size;
            for (size_t i = 0; i < locals->capacity; i++) {
                char *key = locals->keys[i];
                if (key) {
                    Local *other_local = locals->values[i];
                    local->address += other_local->size;
                }
            }
            parser->currentFunction->locals_size += local->size;
            map_set(locals, token->variable, local);
        }
        node->local = local;

        parser_eat(parser, TOKEN_VARIABLE);
        return node;
    }

    print_error(parser->text, token, "Unexpected token: '%s'", token_type_to_string(token->type));
    exit(EXIT_FAILURE);
}
