#include <stdlib.h>
#include <stdio.h> // TODO: remove
#include <assert.h>
#include "parser.h"
#include "vector.h"
#include "token.h"

#define ErrRet                                  \
    rewind_state(parser, _parser_state);        \
    return res;

#define ErrProp                                 \
    if (res.result == PARSER_ERROR) {ErrRet}

static ParserResult parse_transition_unit(Parser *parser);
static ParserResult parse_external_declaration(Parser *parser);
static ParserResult parse_function_definition(Parser *parser);
static ParserResult parse_declaration_specifiers(Parser *parser);
static ParserResult parse_declarator(Parser *parser);
static ParserResult parse_direct_declarator(Parser *parser);
static ParserResult parse_parameter_type_list(Parser *parser);
static ParserResult parse_parameter_list(Parser *parser);
static ParserResult parse_parameter_declaration(Parser *parser);
static ParserResult parse_stmt(Parser *parser);
static ParserResult parse_stmt_compound(Parser *parser);
static ParserResult parse_block_item(Parser *parser);
static ParserResult parse_stmt_expr(Parser *parser);
static ParserResult parse_stmt_jump(Parser *parser);
static ParserResult parse_expr(Parser *parser);
static ParserResult parse_expr_assign(Parser *parser);
static ParserResult parse_expr_additive(Parser *parser);
static ParserResult parse_expr_postfix(Parser *parser);
static ParserResult parse_argument_expr_list(Parser *parser);
static ParserResult parse_expr_primary(Parser *parser);
static ParserResult parse_constant(Parser *parser);
static ParserResult parse_id(Parser *parser);

static ParserResult combinator_more1(Parser *parser, ParserResult (*f)(Parser *parser));
static ParserResult combinator_more1_sep(Parser *parser, TokenKind sep, ParserResult (*f)(Parser *parser));

static ParserResult current_token(Parser *parser);
static ParserResult assume_token(Parser *parser, TokenKind kind);
static void forward_token(Parser *parser);
static void debug_print_current_token(Parser *parser);

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
    ParserResult res;
    state_t _parser_state = save_state(parser);

    res = combinator_more1(parser, parse_external_declaration); ErrProp;
    Vector* nodes = res.value.nodes;

    Node* node = node_arena_malloc(parser->arena);
    node->kind = NODE_TRANS_UNIT;
    node->value.trans_unit.decls = nodes;

    res.value.node = node;

    return res;
}

ParserResult parse_external_declaration(Parser *parser) {
    return parse_function_definition(parser);
}

ParserResult parse_function_definition(Parser *parser) {
    ParserResult res;
    state_t _parser_state = save_state(parser);

    res = parse_id(parser); ErrProp; // TODO: fix
    Node* decl_spec = res.value.node;

    res = parse_declarator(parser); ErrProp;
    Node* decl = res.value.node;

    res = parse_stmt_compound(parser); ErrProp;
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

ParserResult parse_declaration_specifiers(Parser *parser) {
    return parse_id(parser); // TODO: fix
}

ParserResult parse_declarator(Parser *parser) {
    ParserResult res;
    state_t _parser_state = save_state(parser);

    res = parse_direct_declarator(parser); ErrProp;

    Node* node = node_arena_malloc(parser->arena);
    node->kind = NODE_DECLARATOR;
    node->value.declarator.node = res.value.node;

    res.result = PARSER_OK;
    res.value.node = node;

    return res;
}

ParserResult parse_direct_declarator(Parser *parser) {
    ParserResult res;
    state_t _parser_state = save_state(parser);

    Node* gen_node;

    res = parse_id(parser);
    if (res.result == PARSER_ERROR) {
        goto phase2;
    }

    {
        Node* node = node_arena_malloc(parser->arena);
        node->kind = NODE_DIRECT_DECLARATOR;
        node->value.direct_declarator.base = res.value.node;
        node->value.direct_declarator.kind = NODE_DIRECT_DECLARATOR_KIND_BASE;

        gen_node = node;
    }

    goto base_passed;

phase2:
    res = assume_token(parser, TOK_KIND_LPAREN); ErrProp;
    forward_token(parser);

    res = parse_declarator(parser); ErrProp;
    Node* decl = res.value.node;

    res = assume_token(parser, TOK_KIND_RPAREN); ErrProp;
    forward_token(parser);

    {
        Node* node = node_arena_malloc(parser->arena);
        node->kind = NODE_DIRECT_DECLARATOR;
        node->value.direct_declarator.base = decl;
        node->value.direct_declarator.kind = NODE_DIRECT_DECLARATOR_KIND_BASE;

        gen_node = node;
    }

base_passed:
    for(;;) {
        _parser_state = save_state(parser);

        res = current_token(parser);
        if (res.result == PARSER_ERROR) {
            rewind_state(parser, _parser_state);
            goto total_passed;
        }
        forward_token(parser);

        NodeDirectDeclaratorKind d;
        Node* nodes[2];
        switch(res.value.token->kind) {
        case TOK_KIND_LPAREN:
            res = parse_parameter_type_list(parser);
            if (res.result == PARSER_OK) {
                d = NODE_DIRECT_DECLARATOR_KIND_PARAM_TYPE_LIST;
                nodes[0] = res.value.node;
                goto partial_paren_passed;
            }
            goto total_passed; // Not matched

        partial_paren_passed:
            res = assume_token(parser, TOK_KIND_RPAREN);
            if (res.result == PARSER_ERROR) {
                goto total_passed;
            }
            forward_token(parser);
            break;

        case TOK_KIND_LBRACKET:
            res = assume_token(parser, TOK_KIND_RBRACKET);
            if (res.result == PARSER_ERROR) {
                goto total_passed;
            }
            forward_token(parser);
            break;

        default:
            goto total_passed;
        }

        Node* node = node_arena_malloc(parser->arena);
        node->kind = NODE_DIRECT_DECLARATOR;
        node->value.direct_declarator.kind = d;
        node->value.direct_declarator.base = gen_node;

        switch(d) {
        case NODE_DIRECT_DECLARATOR_KIND_PARAM_TYPE_LIST:
            node->value.direct_declarator.args[0] = nodes[0];
            break;
        default:
            assert(0); // TODO: error handling...
        }

        gen_node = node;
    }

total_passed:
    rewind_state(parser, _parser_state);

    res.result = PARSER_OK;
    res.value.node = gen_node;

    return res;
}

// TODO: support '...'
ParserResult parse_parameter_type_list(Parser *parser) {
    return parse_parameter_list(parser);
}

ParserResult parse_parameter_list(Parser *parser) {
    ParserResult res;
    state_t _parser_state = save_state(parser);

    res = combinator_more1_sep(parser, TOK_KIND_COMMA, parse_parameter_declaration); ErrProp;

    Node* node = node_arena_malloc(parser->arena);
    node->kind = NODE_PARAM_LIST;
    node->value.param_list.params = res.value.nodes;

    res.result = PARSER_OK;
    res.value.node = node;

    return res;
}

ParserResult parse_parameter_declaration(Parser *parser) {
    ParserResult res;
    state_t _parser_state = save_state(parser);

    res = parse_declaration_specifiers(parser); ErrProp;
    Node* spec = res.value.node;

    Node* node = node_arena_malloc(parser->arena);
    node->kind = NODE_PARAM_DECL;
    node->value.param_decl.spec = spec;

    res = parse_declarator(parser);
    if (res.result == PARSER_ERROR) {
        goto next_phase;
    }
    node->value.param_decl.decl = res.value.node;

    res.result = PARSER_OK;
    res.value.node = node;

    return res;

next_phase:
    // TODO: support 'abstract-declarator opt'
    node->value.param_decl.decl = NULL;

    res.result = PARSER_OK;
    res.value.node = node;

    return res;
}

ParserResult parse_stmt(Parser *parser) {
    ParserResult res;

    res = parse_stmt_compound(parser);
    if (res.result == PARSER_OK) {
        goto finish;
    }

    res = parse_stmt_expr(parser);
    if (res.result == PARSER_OK) {
        goto finish;
    }

    res = parse_stmt_jump(parser);
    if (res.result == PARSER_OK) {
        goto finish;
    }

    // Not matched...
    res.result = PARSER_ERROR;
    res.error.kind = PARSER_ERROR_KIND_UNEXPECTED; // TODO: fix
    printf("failed: parse stmt\n");

finish:
    return res;
}

ParserResult parse_stmt_compound(Parser *parser) {
    ParserResult res;
    state_t _parser_state = save_state(parser);

    res = assume_token(parser, TOK_KIND_LBRACE); ErrProp;
    forward_token(parser);

    res = combinator_more1(parser, parse_block_item); ErrProp;
    Vector* nodes = res.value.nodes;

    res = assume_token(parser, TOK_KIND_RBRACE); ErrProp;
    forward_token(parser);

    Node* node = node_arena_malloc(parser->arena);
    node->kind = NODE_STMT_COMPOUND;
    node->value.stmt_compound.stmts = nodes;

    res.result = PARSER_OK;
    res.value.node = node;

    return res;
}

ParserResult parse_block_item(Parser *parser) {
    ParserResult res;

    res = parse_stmt(parser);
    if (res.result == PARSER_OK) {
        goto finish;
    }

    // Not matched...
    res.result = PARSER_ERROR;
    res.error.kind = PARSER_ERROR_KIND_UNEXPECTED; // TODO: fix

finish:
    return res;
}

ParserResult parse_stmt_expr(Parser *parser) {
    printf("parse_stmt_expr: START!\n");
    ParserResult res;
    state_t _parser_state = save_state(parser);

    Node* expr = NULL;
    res = parse_expr(parser); // Optional
    if (res.result == PARSER_OK) {
        printf("parse_stmt_expr: EXPR OK!\n");
        expr = res.value.node;
    }

    printf("parse_stmt_expr: EXPR!\n");
    debug_print_current_token(parser);

    res = assume_token(parser, TOK_KIND_SEMICOLON); ErrProp;
    forward_token(parser);

    Node* node = node_arena_malloc(parser->arena);
    node->kind = NODE_STMT_EXPR;
    node->value.stmt_expr.expr = expr;

    res.result = PARSER_OK;
    res.value.node = node;

    return res;
}

ParserResult parse_stmt_jump(Parser *parser) {
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
    return parse_expr_assign(parser);
}

ParserResult parse_expr_assign(Parser *parser) {
    return parse_expr_additive(parser);
}

ParserResult parse_expr_additive(Parser *parser) {
    ParserResult res;
    state_t _parser_state = save_state(parser);

    res = parse_expr_postfix(parser); ErrProp;

    for (;;) {
        _parser_state = save_state(parser);

        ParserResult res0 = current_token(parser);
        if (res0.result == PARSER_ERROR) {
            rewind_state(parser, _parser_state);
            goto escape;
        }
        Token* op = res0.value.token;

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

        res0 = parse_expr_postfix(parser);
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

ParserResult parse_expr_postfix(Parser *parser) {
    ParserResult res;
    state_t _parser_state = save_state(parser);

    Node* gen_node;

    res = parse_expr_primary(parser); ErrProp;
    gen_node = res.value.node;

    for(;;) {
        _parser_state = save_state(parser);

        res = current_token(parser);
        if (res.result == PARSER_ERROR) {
            rewind_state(parser, _parser_state);
            goto total_passed;
        }
        forward_token(parser);

        NodeExprPostfixKind d;
        Node* nodes[1];
        switch(res.value.token->kind) {
        case TOK_KIND_LPAREN:
            res = parse_argument_expr_list(parser); // Optional
            d = NODE_EXPR_POSTFIX_KIND_FUNC_CALL;
            nodes[0] = NULL;
            if (res.result == PARSER_OK) {
                nodes[0] = res.value.node;
            }

            res = assume_token(parser, TOK_KIND_RPAREN);
            if (res.result == PARSER_ERROR) {
                goto total_passed;
            }
            forward_token(parser);
            break;

        case TOK_KIND_LBRACKET:
            res = assume_token(parser, TOK_KIND_RBRACKET);
            if (res.result == PARSER_ERROR) {
                goto total_passed;
            }
            forward_token(parser);
            break;

        default:
            goto total_passed;
        }

        Node* node = node_arena_malloc(parser->arena);
        node->kind = NODE_EXPR_POSTFIX;
        node->value.expr_postfix.kind = d;
        node->value.expr_postfix.lhs = gen_node;

        switch(d) {
        case NODE_EXPR_POSTFIX_KIND_FUNC_CALL:
            node->value.expr_postfix.rhs = nodes[0];
            break;
        default:
            assert(0); // TODO: error handling...
        }

        gen_node = node;
    }

total_passed:
    rewind_state(parser, _parser_state);

    res.result = PARSER_OK;
    res.value.node = gen_node;

    return res;
}

ParserResult parse_argument_expr_list(Parser *parser) {
    ParserResult res;
    state_t _parser_state = save_state(parser);

    res = combinator_more1_sep(parser, TOK_KIND_COMMA, parse_expr_assign); ErrProp;

    Node* node = node_arena_malloc(parser->arena);
    node->kind = NODE_ARGS_LIST;
    node->value.args_list.args = res.value.nodes;

    res.result = PARSER_OK;
    res.value.node = node;

    return res;
}

ParserResult parse_expr_primary(Parser *parser) {
    ParserResult res;

    res = parse_id(parser);
    if (res.result == PARSER_OK) {
        goto finish;
    }

    res = parse_constant(parser);
    if (res.result == PARSER_OK) {
        goto finish;
    }

    // Not matched...
    res.result = PARSER_ERROR;
    res.error.kind = PARSER_ERROR_KIND_UNEXPECTED; // TODO: fix

finish:
    return res;
}

ParserResult parse_constant(Parser *parser) {
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

    case TOK_KIND_STRING_LIT:
    {
        char* const tok_buf = token_to_string(t);

        Node* node = node_arena_malloc(parser->arena);
        node->kind = NODE_LIT_STRING;
        node->value.lit_string.v = tok_buf;

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

    debug_print_current_token(parser);

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
    return combinator_more1_sep(parser, TOK_KIND_EMPTY, f);
}

ParserResult combinator_more1_sep(Parser *parser, TokenKind sep, ParserResult (*f)(Parser *parser)) {
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

void debug_print_current_token(Parser *parser) {
    fprintf(stderr, "DEBUG_PRINT: CURRENT_TOKEN ");

    ParserResult res = current_token(parser);
    if (res.result == PARSER_ERROR) {
        fprintf(stderr, "ERR!\n");
        return;
    }

    fprintf(stderr, "OK! -> ");
    token_fprint_buf(stderr, res.value.token);
    fprintf(stderr, "\n");
}
