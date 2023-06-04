#include "parser.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

// Type
Type *type_new(TypeKind kind, size_t size) {
    Type *type = malloc(sizeof(Type));
    type->kind = kind;
    type->size = size;
    return type;
}

Type *type_new_pointer(Type *base) {
    Type *type = type_new(TYPE_POINTER, 8);
    type->base = base;
    return type;
}

Type *type_new_array(Type *base, size_t size) {
    Type *type = type_new(TYPE_ARRAY, base->size * size);
    type->base = base;
    return type;
}

void type_dump(FILE *f, Type *type) {
    if (type->kind == TYPE_INTEGER) {
        fprintf(f, "int%zu_t", type->size * 8);
    }
    if (type->kind == TYPE_POINTER) {
        type_dump(f, type->base);
        fprintf(f, "*");
    }
    if (type->kind == TYPE_ARRAY) {
        type_dump(f, type->base);
        fprintf(f, "[%zu]", type->size / type->base->size);
    }
}

// Node
Node *node_new(NodeKind kind, Token *token) {
    Node *node = malloc(sizeof(Node));
    node->kind = kind;
    node->token = token;
    return node;
}

Node *node_new_integer(Token *token, int64_t integer) {
    Node *node = node_new(NODE_INTEGER, token);
    node->type = type_new(TYPE_INTEGER, 8);
    node->integer = integer;
    return node;
}

Node *node_new_unary(NodeKind kind, Token *token, Node *unary) {
    Node *node = node_new(kind, token);
    node->type = unary->type;
    node->unary = unary;
    return node;
}

Node *node_new_operation(NodeKind kind, Token *token, Node *lhs, Node *rhs) {
    Node *node = node_new(kind, token);
    node->type = lhs->type;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *node_new_nodes(NodeKind kind, Token *token) {
    Node *node = node_new(kind, token);
    node->type = NULL;
    node->nodes.capacity = 8;
    list_init(&node->nodes);
    return node;
}

Node *node_new_function(NodeKind kind, Token *token) {
    Node *node = node_new_nodes(kind, token);
    node->locals.capacity = 8;
    list_init(&node->locals);
    node->locals_size = 0;
    return node;
}

Local *node_find_local(Node *node, char *name) {
    for (size_t i = 0; i < node->locals.size; i++) {
        Local *local = node->locals.items[i];
        if (!strcmp(local->name, name)) {
            return local;
        }
    }
    return NULL;
}

void node_dump(FILE *f, Node *node, int32_t indent) {
    if (node->kind == NODE_FUNCTION) {
        fprintf(f, "{\n");
        for (size_t i = 0; i < node->locals.size; i++) {
            Local *local = node->locals.items[i];
            for (int32_t i = 0; i < indent + 1; i++) fprintf(f, "  ");
            type_dump(f, local->type);
            fprintf(f, " %s;\n", local->name);
        }
        for (size_t i = 0; i < node->nodes.size; i++) {
            Node *child = node->nodes.items[i];
            for (int32_t i = 0; i < indent + 1; i++) fprintf(f, "  ");
            node_dump(f, child, indent + 1);
            fprintf(f, ";\n");
        }
        fprintf(f, "}\n");
    }
    if (node->kind == NODE_NODES) {
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

    if (node->kind == NODE_LOCAL) {
        fprintf(f, "%s", node->local->name);
    }
    if (node->kind == NODE_INTEGER) {
        fprintf(f, "%" PRIi64, node->integer);
    }

    if (node->kind == NODE_TENARY) {
        fprintf(f, "( ");
        node_dump(f, node->condition, indent);
        fprintf(f, " ? ");
        node_dump(f, node->then_block, indent);
        fprintf(f, " : ");
        node_dump(f, node->else_block, indent);
        fprintf(f, " )");
    }
    if (node->kind == NODE_IF) {
        fprintf(f, "if (");
        node_dump(f, node->condition, indent);
        fprintf(f, ") ");
        node_dump(f, node->then_block, indent + 1);
        if (node->else_block != NULL) {
            for (int32_t i = 0; i < indent; i++) fprintf(f, "  ");
            fprintf(f, " else ");
            node_dump(f, node->else_block, indent + 1);
        }
    }
    if (node->kind == NODE_WHILE) {
        fprintf(f, "while (");
        node_dump(f, node->condition, indent);
        fprintf(f, ") ");
        node_dump(f, node->then_block, indent + 1);
    }
    if (node->kind == NODE_DOWHILE) {
        fprintf(f, "do ");
        node_dump(f, node->then_block, indent + 1);
        for (int32_t i = 0; i < indent; i++) fprintf(f, "  ");
        fprintf(f, "while (");
        node_dump(f, node->condition, indent);
        fprintf(f, ")");
    }
    if (node->kind == NODE_RETURN) {
        fprintf(f, "return ");
        node_dump(f, node->unary, indent);
    }

    if (node->kind > NODE_UNARY_BEGIN && node->kind < NODE_UNARY_END) {
        fprintf(f, "( ");

        if (node->kind == NODE_NEG) fprintf(f, "- ");
        if (node->kind == NODE_NOT) fprintf(f, "~ ");
        if (node->kind == NODE_LOGICAL_NOT) fprintf(f, "! ");
        if (node->kind == NODE_ADDR) fprintf(f, "& ");
        if (node->kind == NODE_DEREF) fprintf(f, "* ");

        node_dump(f, node->unary, indent);
        fprintf(f, " )");
    }

    if (node->kind > NODE_OPERATION_BEGIN && node->kind < NODE_OPERATION_END) {
        fprintf(f, "( ");
        node_dump(f, node->lhs, indent);

        if (node->kind == NODE_ASSIGN) fprintf(f, " = ");
        if (node->kind == NODE_ADD) fprintf(f, " + ");
        if (node->kind == NODE_SUB) fprintf(f, " - ");
        if (node->kind == NODE_MUL) fprintf(f, " * ");
        if (node->kind == NODE_DIV) fprintf(f, " / ");
        if (node->kind == NODE_MOD) fprintf(f, " %% ");
        if (node->kind == NODE_AND) fprintf(f, " & ");
        if (node->kind == NODE_OR) fprintf(f, " | ");
        if (node->kind == NODE_XOR) fprintf(f, " ^ ");
        if (node->kind == NODE_SHL) fprintf(f, " << ");
        if (node->kind == NODE_SHR) fprintf(f, " >> ");
        if (node->kind == NODE_EQ) fprintf(f, " == ");
        if (node->kind == NODE_NEQ) fprintf(f, " != ");
        if (node->kind == NODE_LT) fprintf(f, " < ");
        if (node->kind == NODE_LTEQ) fprintf(f, " <= ");
        if (node->kind == NODE_GT) fprintf(f, " > ");
        if (node->kind == NODE_GTEQ) fprintf(f, " >= ");
        if (node->kind == NODE_LOGICAL_AND) fprintf(f, " && ");
        if (node->kind == NODE_LOGICAL_OR) fprintf(f, " || ");

        node_dump(f, node->rhs, indent);
        fprintf(f, " )");
    }
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

void parser_eat(Parser *parser, TokenKind token_kind) {
    Token *token = current();
    if (token->kind == token_kind) {
        parser->position++;
    } else {
        print_error(parser->text, token, "Unexpected token: '%s' wanted '%s'", token_kind_to_string(token->kind), token_kind_to_string(token_kind));
        exit(EXIT_FAILURE);
    }
}

Type *parser_type(Parser *parser) {
    Type *type;
    if (current()->kind == TOKEN_INT) {
        parser_eat(parser, TOKEN_INT);
        type = type_new(TYPE_INTEGER, 8);
    }

    while (current()->kind == TOKEN_MUL) {
        parser_eat(parser, TOKEN_MUL);
        type = type_new_pointer(type);
    }
    return type;
}

Type *parser_type_suffix(Parser *parser, Type *type) {
    while (current()->kind == TOKEN_LBLOCK) {
        parser_eat(parser, TOKEN_LBLOCK);
        type = type_new_array(type, current()->integer);
        parser_eat(parser, TOKEN_INTEGER);
        parser_eat(parser, TOKEN_RBLOCK);
    }
    return type;
}

Node *parser_add_node(Parser *parser, Token *token, Node *lhs, Node *rhs) {
    if (lhs->type->kind == TYPE_INTEGER && rhs->type->kind == TYPE_INTEGER) {
        return node_new_operation(NODE_ADD, token, lhs, rhs);
    }
    if (lhs->type->kind == TYPE_INTEGER && (rhs->type->kind == TYPE_POINTER || rhs->type->kind == TYPE_ARRAY)) {
        Node *tmp = rhs;
        rhs = lhs;
        lhs = tmp;
    }
    if ((lhs->type->kind == TYPE_POINTER || lhs->type->kind == TYPE_ARRAY) && rhs->type->kind == TYPE_INTEGER) {
        return node_new_operation(NODE_ADD, token, lhs, parser_mul_node(parser, token, rhs, node_new_integer(token, lhs->type->base->size)));
    }

    print_error(parser->text, token, "Invalid add types");
    exit(EXIT_FAILURE);
}

Node *parser_sub_node(Parser *parser, Token *token, Node *lhs, Node *rhs) {
    if (lhs->type->kind == TYPE_INTEGER && rhs->type->kind == TYPE_INTEGER) {
        return node_new_operation(NODE_SUB, token, lhs, rhs);
    }
    if (lhs->type->kind == TYPE_POINTER && rhs->type->kind == TYPE_POINTER) {
        Node *node = parser_div_node(parser, token, node_new_operation(NODE_SUB, token, lhs, rhs), node_new_integer(token, lhs->type->base->size));
        node->type = node->type->base;
        return node;
    }
    if ((lhs->type->kind == TYPE_POINTER || lhs->type->kind == TYPE_ARRAY) && rhs->type->kind == TYPE_INTEGER) {
        return node_new_operation(NODE_SUB, token, lhs, parser_mul_node(parser, token, rhs, node_new_integer(token, lhs->type->base->size)));
    }

    print_error(parser->text, token, "Invalid subtract types");
    exit(EXIT_FAILURE);
}

Node *parser_mul_node(Parser *parser, Token *token, Node *lhs, Node *rhs) {
    (void)parser;
    if (rhs->kind == TYPE_INTEGER && power_of_two(rhs->integer)) {
        rhs->integer = log(rhs->integer) / log(power_of_two(rhs->integer));
        return node_new_operation(NODE_SHL, token, lhs, rhs);
    }
    return node_new_operation(NODE_MUL, token, lhs, rhs);
}

Node *parser_div_node(Parser *parser, Token *token, Node *lhs, Node *rhs) {
    (void)parser;
    if (rhs->kind == TYPE_INTEGER && power_of_two(rhs->integer)) {
        rhs->integer = log(rhs->integer) / log(power_of_two(rhs->integer));
        return node_new_operation(NODE_SHR, token, lhs, rhs);
    }
    return node_new_operation(NODE_DIV, token, lhs, rhs);
}

Node *parser_function(Parser *parser) {
    Token *token = current();
    Node *node = node_new_function(NODE_FUNCTION, token);
    Node *oldFunction = parser->current_function;
    parser->current_function = node;

    parser_eat(parser, TOKEN_LCURLY);
    while (current()->kind != TOKEN_RCURLY) {
        Node *child = parser_statement(parser);
        if (child != NULL) list_add(&node->nodes, child);
    }
    parser_eat(parser, TOKEN_RCURLY);

    // Set locals offset
    size_t local_offset = node->locals_size;
    for (size_t i = 0; i < node->locals.size; i++) {
        Local *local = node->locals.items[i];
        local->offset = local_offset;
        local_offset -= local->type->size;
    }

    parser->current_function = oldFunction;
    return node;
}

Node *parser_block(Parser *parser) {
    Token *token = current();
    Node *node = node_new_nodes(NODE_NODES, token);
    if (token->kind == TOKEN_LCURLY) {
        parser_eat(parser, TOKEN_LCURLY);
        while (current()->kind != TOKEN_RCURLY) {
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

    if (token->kind == TOKEN_SEMICOLON) {
        parser_eat(parser, TOKEN_SEMICOLON);
        return NULL;
    }

    if (token->kind == TOKEN_LCURLY) {
        return parser_block(parser);
    }

    if (token->kind == TOKEN_IF) {
        Node *node = node_new(NODE_IF, token);
        parser_eat(parser, TOKEN_IF);
        parser_eat(parser, TOKEN_LPAREN);
        node->condition = parser_assign(parser);
        parser_eat(parser, TOKEN_RPAREN);
        node->then_block = parser_block(parser);
        if (current()->kind == TOKEN_ELSE) {
            parser_eat(parser, TOKEN_ELSE);
            node->else_block = parser_block(parser);
        } else {
            node->else_block = NULL;
        }
        return node;
    }

    if (token->kind == TOKEN_WHILE) {
        Node *node = node_new(NODE_WHILE, token);
        node->else_block = NULL;
        parser_eat(parser, TOKEN_WHILE);
        parser_eat(parser, TOKEN_LPAREN);
        node->condition = parser_assign(parser);
        parser_eat(parser, TOKEN_RPAREN);
        node->then_block = parser_block(parser);
        return node;
    }

    if (token->kind == TOKEN_DO) {
        Node *node = node_new(NODE_DOWHILE, token);
        node->else_block = NULL;
        parser_eat(parser, TOKEN_DO);
        node->then_block = parser_block(parser);
        parser_eat(parser, TOKEN_WHILE);
        parser_eat(parser, TOKEN_LPAREN);
        node->condition = parser_assign(parser);
        parser_eat(parser, TOKEN_RPAREN);
        parser_eat(parser, TOKEN_SEMICOLON);
        return node;
    }

    if (token->kind == TOKEN_FOR) {
        Node *parent_node = node_new_nodes(NODE_NODES, token);
        Node *node = node_new(NODE_WHILE, token);
        node->then_block = node_new_nodes(NODE_NODES, token);

        parser_eat(parser, TOKEN_FOR);
        parser_eat(parser, TOKEN_LPAREN);

        if (current()->kind != TOKEN_SEMICOLON) {
            list_add(&parent_node->nodes, parser_declarations(parser));
        }
        parser_eat(parser, TOKEN_SEMICOLON);

        list_add(&parent_node->nodes, node);

        if (current()->kind != TOKEN_SEMICOLON) {
            node->condition = parser_assign(parser);
        } else {
            node->condition = NULL;
        }
        parser_eat(parser, TOKEN_SEMICOLON);

        Node *increment_node = NULL;
        if (current()->kind != TOKEN_RPAREN) {
            increment_node = parser_assigns(parser);
        }
        parser_eat(parser, TOKEN_RPAREN);

        list_add(&node->then_block->nodes, parser_block(parser));
        if (increment_node != NULL) {
            list_add(&node->then_block->nodes, increment_node);
        }
        return parent_node;
    }

    if (token->kind == TOKEN_RETURN) {
        parser_eat(parser, TOKEN_RETURN);
        Node *node = node_new_unary(NODE_RETURN, token, parser_assign(parser));
        parser_eat(parser, TOKEN_SEMICOLON);
        return node;
    }

    Node *node = parser_declarations(parser);
    parser_eat(parser, TOKEN_SEMICOLON);
    return node;
}

Node *parser_declarations(Parser *parser) {
    Token *token = current();
    if (token->kind > TOKEN_TYPE_BEGIN && token->kind < TOKEN_TYPE_END) {
        Type *base_type = parser_type(parser);
        List *nodes = list_new();
        for (;;) {
            char *name = current()->variable;
            parser_eat(parser, TOKEN_VARIABLE);
            Type *local_type = parser_type_suffix(parser, base_type);

            // Create local when it doesn't exists
            Local *local = node_find_local(parser->current_function, name);
            if (local == NULL) {
                local = malloc(sizeof(Local));
                local->name = name;
                local->type = local_type;
                list_add(&parser->current_function->locals, local);
                parser->current_function->locals_size += local->type->size;
            } else {
                print_error(parser->text, token, "[TMP] Can't redefine variable: '%s'", name);
                exit(EXIT_FAILURE);
            }

            if (current()->kind == TOKEN_ASSIGN) {
                parser_eat(parser, TOKEN_ASSIGN);
                Node *node = node_new(NODE_LOCAL, token);
                node->local = local;
                list_add(nodes, node_new_operation(NODE_ASSIGN, token, node, parser_assign(parser)));
            }

            if (current()->kind == TOKEN_COMMA) {
                parser_eat(parser, TOKEN_COMMA);
            } else {
                break;
            }
        }
        if (nodes->size == 0) {
            list_free(nodes, NULL);
            return NULL;
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
    return parser_assigns(parser);
}

Node *parser_assigns(Parser *parser) {
    Token *token = current();
    List *nodes = list_new();
    for (;;) {
        list_add(nodes, parser_assign(parser));
        if (current()->kind == TOKEN_COMMA) {
            parser_eat(parser, TOKEN_COMMA);
        } else {
            break;
        }
    }
    if (nodes->size == 0) {
        list_free(nodes, NULL);
        return NULL;
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
    Node *lhs = parser_tenary(parser);
    if (current()->kind > TOKEN_ASSIGN_BEGIN && current()->kind < TOKEN_ASSIGN_END) {
        Token *token = current();
        if (lhs->type->kind == TYPE_ARRAY) {
            print_error(parser->text, token, "Can't assign an array type");
        }

        if (token->kind == TOKEN_ASSIGN) {
            parser_eat(parser, TOKEN_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, parser_assign(parser));
        }
        if (token->kind == TOKEN_ADD_ASSIGN) {
            parser_eat(parser, TOKEN_ADD_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, parser_add_node(parser, token, lhs, parser_assign(parser)));
        }
        if (token->kind == TOKEN_SUB_ASSIGN) {
            parser_eat(parser, TOKEN_SUB_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, parser_sub_node(parser, token, lhs, parser_assign(parser)));
        }
        if (token->kind == TOKEN_MUL_ASSIGN) {
            parser_eat(parser, TOKEN_MUL_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, parser_mul_node(parser, token, lhs, parser_assign(parser)));
        }
        if (token->kind == TOKEN_DIV_ASSIGN) {
            parser_eat(parser, TOKEN_DIV_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, parser_div_node(parser, token, lhs, parser_assign(parser)));
        }
        if (token->kind == TOKEN_MOD_ASSIGN) {
            parser_eat(parser, TOKEN_MOD_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, node_new_operation(NODE_MOD, token, lhs, parser_assign(parser)));
        }
        if (token->kind == TOKEN_AND_ASSIGN) {
            parser_eat(parser, TOKEN_AND_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, node_new_operation(NODE_AND, token, lhs, parser_assign(parser)));
        }
        if (token->kind == TOKEN_OR_ASSIGN) {
            parser_eat(parser, TOKEN_OR_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, node_new_operation(NODE_OR, token, lhs, parser_assign(parser)));
        }
        if (token->kind == TOKEN_XOR_ASSIGN) {
            parser_eat(parser, TOKEN_XOR_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, node_new_operation(NODE_XOR, token, lhs, parser_assign(parser)));
        }
        if (token->kind == TOKEN_SHL_ASSIGN) {
            parser_eat(parser, TOKEN_SHL_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, node_new_operation(NODE_SHL, token, lhs, parser_assign(parser)));
        }
        if (token->kind == TOKEN_SHR_ASSIGN) {
            parser_eat(parser, TOKEN_SHR_ASSIGN);
            return node_new_operation(NODE_ASSIGN, token, lhs, node_new_operation(NODE_SHR, token, lhs, parser_assign(parser)));
        }
    }
    return lhs;
}

Node *parser_tenary(Parser *parser) {
    Node *node = parser_logical(parser);
    if (current()->kind == TOKEN_QUESTION) {
        Node *tenary_node = node_new(NODE_TENARY, current());
        parser_eat(parser, TOKEN_QUESTION);
        tenary_node->condition = node;
        tenary_node->then_block = parser_tenary(parser);
        parser_eat(parser, TOKEN_COLON);
        tenary_node->else_block = parser_tenary(parser);
        return tenary_node;
    }
    return node;
}

Node *parser_logical(Parser *parser) {
    Node *node = parser_equality(parser);
    while (current()->kind == TOKEN_LOGICAL_AND || current()->kind == TOKEN_LOGICAL_OR) {
        if (current()->kind == TOKEN_LOGICAL_AND) {
            Token *token = current();
            parser_eat(parser, TOKEN_LOGICAL_AND);
            node = node_new_operation(NODE_LOGICAL_AND, token, node, parser_equality(parser));
        }
        if (current()->kind == TOKEN_LOGICAL_OR) {
            Token *token = current();
            parser_eat(parser, TOKEN_LOGICAL_OR);
            node = node_new_operation(NODE_LOGICAL_OR, token, node, parser_equality(parser));
        }
    }
    return node;
}

Node *parser_equality(Parser *parser) {
    Node *node = parser_relational(parser);
    while (current()->kind == TOKEN_EQ || current()->kind == TOKEN_NEQ) {
        Token *token = current();
        if (token->kind == TOKEN_EQ) {
            parser_eat(parser, TOKEN_EQ);
            node = node_new_operation(NODE_EQ, token, node, parser_relational(parser));
        }
        if (token->kind == TOKEN_NEQ) {
            parser_eat(parser, TOKEN_NEQ);
            node = node_new_operation(NODE_NEQ, token, node, parser_relational(parser));
        }
    }
    return node;
}

Node *parser_relational(Parser *parser) {
    Node *node = parser_bitwise(parser);
    while (current()->kind == TOKEN_LT || current()->kind == TOKEN_LTEQ || current()->kind == TOKEN_GT || current()->kind == TOKEN_GTEQ) {
        Token *token = current();
        if (token->kind == TOKEN_LT) {
            parser_eat(parser, TOKEN_LT);
            node = node_new_operation(NODE_LT, token, node, parser_bitwise(parser));
        }
        if (token->kind == TOKEN_LTEQ) {
            parser_eat(parser, TOKEN_LTEQ);
            node = node_new_operation(NODE_LTEQ, token, node, parser_bitwise(parser));
        }
        if (token->kind == TOKEN_GT) {
            parser_eat(parser, TOKEN_GT);
            node = node_new_operation(NODE_GT, token, node, parser_bitwise(parser));
        }
        if (token->kind == TOKEN_GTEQ) {
            parser_eat(parser, TOKEN_GTEQ);
            node = node_new_operation(NODE_GTEQ, token, node, parser_bitwise(parser));
        }
    }
    return node;
}

Node *parser_bitwise(Parser *parser) {
    Node *node = parser_shift(parser);
    while (current()->kind == TOKEN_AND || current()->kind == TOKEN_XOR || current()->kind == TOKEN_OR) {
        Token *token = current();
        if (token->kind == TOKEN_AND) {
            parser_eat(parser, TOKEN_AND);
            node = node_new_operation(NODE_AND, token, node, parser_shift(parser));
        }
        if (token->kind == TOKEN_XOR) {
            parser_eat(parser, TOKEN_XOR);
            node = node_new_operation(NODE_XOR, token, node, parser_shift(parser));
        }
        if (token->kind == TOKEN_OR) {
            parser_eat(parser, TOKEN_OR);
            node = node_new_operation(NODE_OR, token, node, parser_shift(parser));
        }
    }
    return node;
}

Node *parser_shift(Parser *parser) {
    Node *node = parser_add(parser);
    while (current()->kind == TOKEN_SHL || current()->kind == TOKEN_SHR) {
        Token *token = current();
        if (token->kind == TOKEN_SHL) {
            parser_eat(parser, TOKEN_SHL);
            node = node_new_operation(NODE_SHL, token, node, parser_add(parser));
        }
        if (token->kind == TOKEN_SHR) {
            parser_eat(parser, TOKEN_SHR);
            node = node_new_operation(NODE_SHR, token, node, parser_add(parser));
        }
    }
    return node;
}

Node *parser_add(Parser *parser) {
    Node *node = parser_mul(parser);
    while (current()->kind == TOKEN_ADD || current()->kind == TOKEN_SUB) {
        Token *token = current();
        if (token->kind == TOKEN_ADD) {
            parser_eat(parser, TOKEN_ADD);
            node = parser_add_node(parser, token, node, parser_mul(parser));
        }
        if (token->kind == TOKEN_SUB) {
            parser_eat(parser, TOKEN_SUB);
            node = parser_sub_node(parser, token, node, parser_mul(parser));
        }
    }
    return node;
}

Node *parser_mul(Parser *parser) {
    Node *node = parser_unary(parser);
    while (current()->kind == TOKEN_MUL || current()->kind == TOKEN_DIV || current()->kind == TOKEN_MOD) {
        Token *token = current();
        if (token->kind == TOKEN_MUL) {
            parser_eat(parser, TOKEN_MUL);
            node = parser_mul_node(parser, token, node, parser_unary(parser));
        }
        if (token->kind == TOKEN_DIV) {
            parser_eat(parser, TOKEN_DIV);
            node = parser_div_node(parser, token, node, parser_unary(parser));
        }
        if (token->kind == TOKEN_MOD) {
            parser_eat(parser, TOKEN_MOD);
            node = node_new_operation(NODE_MOD, token, node, parser_unary(parser));
        }
    }
    return node;
}

Node *parser_unary(Parser *parser) {
    Token *token = current();
    if (token->kind == TOKEN_ADD) {
        parser_eat(parser, TOKEN_ADD);
        return parser_unary(parser);
    }
    if (token->kind == TOKEN_SUB) {
        parser_eat(parser, TOKEN_SUB);
        return node_new_unary(NODE_NEG, token, parser_unary(parser));
    }
    if (token->kind == TOKEN_NOT) {
        parser_eat(parser, TOKEN_NOT);
        return node_new_unary(NODE_NOT, token, parser_unary(parser));
    }
    if (token->kind == TOKEN_LOGICAL_NOT) {
        parser_eat(parser, TOKEN_LOGICAL_NOT);
        return node_new_unary(NODE_LOGICAL_NOT, token, parser_unary(parser));
    }
    if (token->kind == TOKEN_AND) {
        parser_eat(parser, TOKEN_AND);
        Node *node = node_new_unary(NODE_ADDR, token, parser_unary(parser));
        if (node->type->kind == TYPE_ARRAY) {
            node->type = type_new_pointer(node->type->base);
        } else {
            node->type = type_new_pointer(node->type);
        }
        return node;
    }
    if (token->kind == TOKEN_MUL) {
        parser_eat(parser, TOKEN_MUL);
        Node *node = node_new_unary(NODE_DEREF, token, parser_unary(parser));
        if (node->type->kind != TYPE_POINTER && node->type->kind != TYPE_ARRAY) {
            print_error(parser->text, token, "Type is not a pointer");
            exit(EXIT_FAILURE);
        }
        node->type = node->type->base;
        return node;
    }
    if (token->kind == TOKEN_SIZEOF) {
        parser_eat(parser, TOKEN_SIZEOF);
        Node *node = parser_unary(parser);
        return node_new_integer(token, node->type->size);
    }
    return parser_primary(parser);
}

Node *parser_primary(Parser *parser) {
    Token *token = current();

    if (token->kind == TOKEN_LPAREN) {
        parser_eat(parser, TOKEN_LPAREN);
        Node *node = parser_assign(parser);
        parser_eat(parser, TOKEN_RPAREN);
        return node;
    }
    if (token->kind == TOKEN_INTEGER) {
        Node *node = node_new_integer(token, token->integer);
        parser_eat(parser, TOKEN_INTEGER);
        return node;
    }
    if (token->kind == TOKEN_VARIABLE) {
        Node *node = node_new(NODE_LOCAL, token);
        Local *local = node_find_local(parser->current_function, token->variable);
        if (local == NULL) {
            print_error(parser->text, token, "Undefined variable: '%s'", token->variable);
            exit(EXIT_FAILURE);
        }
        node->local = local;
        node->type = local->type;
        parser_eat(parser, TOKEN_VARIABLE);
        return node;
    }

    print_error(parser->text, token, "Unexpected token: '%s'", token_kind_to_string(token->kind));
    exit(EXIT_FAILURE);
}
