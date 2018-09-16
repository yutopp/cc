#include <stdlib.h>
#include <stdio.h> // TODO: remove
#include "parser.h"
#include "vector.h"
#include "token.h"

#define ErrRet                                  \
    rewind_state(parser, _parser_state);        \
    return res;                                 \

#define ErrProp                                 \
    if (res.result == PARSER_ERROR) {ErrRet}    \

static ParserResult parse_transition_unit(Parser *parser);
static ParserResult parse_external_declaration(Parser *parser);
static ParserResult parse_function_definition(Parser *parser);
static ParserResult parse_compound_stmt(Parser *parser);
static ParserResult parse_jump_stmt(Parser *parser);
static ParserResult parse_expr(Parser *parser);
static ParserResult parse_expr_additive(Parser *parser);
static ParserResult parse_expr_primary(Parser *parser);
static ParserResult parse_lit(Parser *parser);
static ParserResult parse_id(Parser *parser);

static ParserResult combinator_more1(Parser *parser, ParserResult (*f)(Parser *parser));

static ParserResult current_token(Parser *parser);
static ParserResult assume_token(Parser *parser, TokenKind kind);
static void forward_token(Parser *parser);

typedef size_t state_t;
static state_t save_state(Parser *parser);
static void rewind_state(Parser *parser, state_t state);

struct parser_t {
    Vector* tokens;
    NodeArena* arena;

    size_t position;
    Vector* position_stack;
};

Parser* parser_new(Vector* tokens, NodeArena* arena) {
    Parser* p = (Parser*)malloc(sizeof(Parser));
    if (!p) {
        return 0;
    }
    p->tokens = tokens;
    p->arena = arena;
    p->position = 0;
    p->position_stack = vector_new(sizeof(size_t));

    return p;
}

void parser_drop(Parser *parser) {
    if (!parser) {
        return;
    }
    vector_drop(parser->position_stack);

    free(parser);
}

ParserResult parser_parse(Parser *parser) {
    ParserResult res;
    state_t _parser_state = save_state(parser);

    res = parse_transition_unit(parser); ErrProp;

    res.result = PARSER_OK;

    return res;
}

void parser_fprint_error(FILE *fp, ParseError *err) {
    switch (err->kind) {
    case PARSER_ERROR_KIND_EOF:
        fprintf(fp, "Unexpected EOF");
        break;

    case PARSER_ERROR_KIND_UNEXPECTED:
        fprintf(fp, "Unexpected token: ");
        token_fprint(fp, err->value.unexpected.token);
        break;

    case PARSER_ERROR_KIND_MORE1:
        fprintf(fp, "Eexpected more than 0 tokens");
        break;
    }
}

ParserResult parse_transition_unit(Parser *parser) {
    return parse_external_declaration(parser);
}

ParserResult parse_external_declaration(Parser *parser) {
    return parse_function_definition(parser);
}

ParserResult parse_function_definition(Parser *parser) {
    ParserResult res;
    state_t _parser_state = save_state(parser);

    res = parse_id(parser); ErrProp; // TODO: fix
    Node* decl_spec = res.value.node;

    res = parse_id(parser); ErrProp; // TODO: fix
    Node* decl = res.value.node;

    res = assume_token(parser, TOK_KIND_LPAREN); ErrProp;
    forward_token(parser);

    res = assume_token(parser, TOK_KIND_ID); ErrProp;
    Token* v0 = res.value.token;
    forward_token(parser);

    res = assume_token(parser, TOK_KIND_RPAREN); ErrProp;
    forward_token(parser);

    res = parse_compound_stmt(parser); ErrProp;
    Node* block = res.value.node;

    Node* node = node_arena_malloc(parser->arena);
    node->kind = NODE_FUNC_DEF;
    node->value.func_def.decl_spec = decl_spec;
    node->value.func_def.decl = decl;
    node->value.func_def.block = block;

    res.result = PARSER_OK;
    res.value.node = node;

    return res;
}

ParserResult parse_compound_stmt(Parser *parser) {
    ParserResult res;
    state_t _parser_state = save_state(parser);

    res = assume_token(parser, TOK_KIND_LBLOCK); ErrProp;
    forward_token(parser);

    res = combinator_more1(parser, parse_jump_stmt); ErrProp;
    Vector* nodes = res.value.nodes;

    res = assume_token(parser, TOK_KIND_RBLOCK); ErrProp;
    forward_token(parser);

    Node* node = node_arena_malloc(parser->arena);
    node->kind = NODE_STMT_COMPOUND;
    node->value.stmt_compound.stmts = nodes;

    res.result = PARSER_OK;
    res.value.node = node;

    return res;
}

ParserResult parse_jump_stmt(Parser *parser) {
    ParserResult res;
    state_t _parser_state = save_state(parser);

    res = current_token(parser); ErrProp;
    forward_token(parser);

    Token* t = res.value.token;
    switch (t->kind) {
    case TOK_KIND_RETURN:
        res = parse_expr(parser); ErrProp;
        Node* expr = res.value.node;

        res = assume_token(parser, TOK_KIND_SEMICOLON); ErrProp;
        forward_token(parser);

        Node* node = node_arena_malloc(parser->arena);
        node->kind = NODE_STMT_JUMP;
        node->value.stmt_jump.kind = t->kind;
        node->value.stmt_jump.expr = expr;

        res.result = PARSER_OK;
        res.value.node = node;

        return res;

    default:
        res.result = PARSER_ERROR;
        res.error.kind = PARSER_ERROR_KIND_UNEXPECTED;
        res.error.value.unexpected.token = t;

        rewind_state(parser, _parser_state);
        return res;
    }
}

ParserResult parse_expr(Parser *parser) {
    return parse_expr_additive(parser);
}

ParserResult parse_expr_additive(Parser *parser) {
    ParserResult res;
    state_t _parser_state = save_state(parser);

    res = parse_expr_primary(parser); ErrProp;
    node_fprint(stdout, res.value.node);

    for (;;) {
        ParserResult res0 = current_token(parser);
        if (res0.result == PARSER_ERROR) {
            rewind_state(parser, _parser_state);
            goto escape;
        }
        Token* op = res0.value.token;
        token_fprint(stdout, op);

        switch (op->kind) {
        case TOK_KIND_PLUS:
            break;
        case TOK_KIND_MINUS:
            break;
        default:
            rewind_state(parser, _parser_state);
            goto escape;
        }
        forward_token(parser);

        res0 = parse_expr_primary(parser);
        if (res0.result == PARSER_ERROR) {
            rewind_state(parser, _parser_state);
            goto escape;
        }

        Node* node = node_arena_malloc(parser->arena);
        node->kind = NODE_EXPR_BIN;
        node->value.expr_bin.op = op;
        node->value.expr_bin.lhs = res.value.node;
        node->value.expr_bin.rhs = res0.value.node;

        res.value.node = node;

        _parser_state = save_state(parser); // Forward state
    }

escape:
    return res;
}

ParserResult parse_expr_primary(Parser *parser) {
    return parse_lit(parser);
}

ParserResult parse_lit(Parser *parser) {
    ParserResult res;
    state_t _parser_state = save_state(parser);

    res = current_token(parser); ErrProp;
    forward_token(parser);

    Token* t = res.value.token;
    switch (t->kind) {
    case TOK_KIND_INT_LIT:
    {
        char* const tok_buf = token_to_string(t);
        long int n = strtol(tok_buf, NULL, 10); // TODO: fix type
        free(tok_buf);

        Node* node = node_arena_malloc(parser->arena);
        node->kind = NODE_LIT_INT;
        node->value.lit_int.v = n;

        res.result = PARSER_OK;
        res.value.node = node;

        return res;
    }

    default:
        res.result = PARSER_ERROR;
        res.error.kind = PARSER_ERROR_KIND_UNEXPECTED;
        res.error.value.unexpected.token = t;

        rewind_state(parser, _parser_state);
        return res;
    }
}

ParserResult parse_id(Parser *parser) {
    ParserResult res;
    state_t _parser_state = save_state(parser);

    res = current_token(parser); ErrProp;
    forward_token(parser);

    Token* t = res.value.token;
    switch (t->kind) {
    case TOK_KIND_ID:
    {
        Node* node = node_arena_malloc(parser->arena);
        node->kind = NODE_ID;
        node->value.id.tok = t;

        res.result = PARSER_OK;
        res.value.node = node;

        return res;
    }

    default:
        res.result = PARSER_ERROR;
        res.error.kind = PARSER_ERROR_KIND_UNEXPECTED;
        res.error.value.unexpected.token = t;

        rewind_state(parser, _parser_state);
        return res;
    }
}

ParserResult combinator_more1(Parser *parser, ParserResult (*f)(Parser *parser)) {
    ParserResult res;
    state_t _parser_state = save_state(parser);

    Vector* elems = vector_new(sizeof(Node*));

    for(;;) {
        res = f(parser);
        if (res.result == PARSER_ERROR) {
            break;
        }

        Node** np = (Node**)vector_append(elems);
        *np = res.value.node;
    }

    if (vector_len(elems) == 0) {
        vector_drop(elems);

        res.result = PARSER_ERROR;
        res.error.kind = PARSER_ERROR_KIND_MORE1;

        rewind_state(parser, _parser_state);
        return res;
    }

    res.result = PARSER_OK;
    res.value.nodes = elems;

    return res;
}

ParserResult current_token(Parser *parser) {
    ParserResult res;

    Token* t = (Token*)vector_at(parser->tokens, parser->position);
    if (!t) {
        res.result = PARSER_ERROR;
        res.error.kind = PARSER_ERROR_KIND_EOF;

        // This function does not consume any tokens, not rewind

        return res;
    }

    res.result = PARSER_OK;
    res.value.token = t;

    return res;
}

ParserResult assume_token(Parser *parser, TokenKind kind) {
    ParserResult res = current_token(parser);
    if (res.result == PARSER_ERROR) {
        return res;
    }

    Token* t = res.value.token;

    if (t->kind != kind) {
        res.result = PARSER_ERROR;
        res.error.kind = PARSER_ERROR_KIND_UNEXPECTED;
        res.error.value.unexpected.token = t;

        // This function does not consume any tokens, not rewind

        return res;
    }

    return res;
}

void forward_token(Parser *parser) {
    parser->position++;
}

state_t save_state(Parser *parser) {
    return parser->position;
}

void rewind_state(Parser *parser, state_t state) {
    parser->position = state;
}
