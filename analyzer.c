#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "analyzer.h"

typedef enum {
    ENV_KIND_TRANS,
} EnvKind;

struct env_t;

typedef struct env_t {
    struct env_t* parent;
    Token* name;
    EnvKind kind;
    union {
    } value;
    Vector* children; // Vector<Env>
} Env;

void env_construct(Env* env, Token* name, Env* parent) {
    env->parent = parent;
    env->name = name;
    env->children = vector_new(sizeof(Env));
}

void env_destruct(Env* env) {
    for(size_t i=0; i<vector_len(env->children); ++i) {
        Env* e = vector_at(env->children, i);
        env_destruct(e);
    }
    vector_drop(env->children);
}

Env* env_new(Token* name, Env* parent) {
    Env* e = (Env*)malloc(sizeof(Env));
    env_construct(e, name, parent);

    return e;
}

void env_drop(Env* env) {
    env_destruct(env);
    free(env);
}

Env* env_insert(Env* env, Env* child) {
    Env* e = vector_append(env->children);
    *e = *child;

    return e;
}

Env* env_lookup(Env* env, Token* name) {
    assert(0); // TODO: implement
    return 0;
}

static void analyze(Analyzer* a, Node* node, Env* env);
static void analyze_expr(Analyzer* a, Node* node, Env* env);

struct analyzer_t {
};

Analyzer* analyzer_new() {
    Analyzer* a = (Analyzer*)malloc(sizeof(Analyzer));

    return a;
}

void analyzer_drop(Analyzer* a) {
    free(a);
}

void analyzer_analyze(Analyzer* a, Node* node) {
    Env* top = env_new(NULL, 0); // TODO: Set a name of the transition unit
    top->kind = ENV_KIND_TRANS;

    analyze(a, node, top);

    env_drop(top);
}

void analyze(Analyzer* a, Node* node, Env* env) {
    switch(node->kind) {
    case NODE_TRANS_UNIT:
    {
        printf("LOG: transition unit\n");

        Vector* decls = node->value.trans_unit.decls;
        for(size_t i=0; i<vector_len(decls); ++i) {
            Node** n = (Node**)vector_at(decls, i);
            analyze(a, *n, env);
        }
        break;
    }

    case NODE_FUNC_DEF:
    {
        printf("LOG: function def\n");

        node_fprint(stdout, node->value.func_def.decl);
        assert(node->value.func_def.decl->kind == NODE_DECLARATOR);
        Token* id_tok = node_declarator_extract_id_token(node->value.func_def.decl);
        assert(id_tok); // TODO: error handling

        token_fprint(stdout, id_tok);

        Env fenv;
        env_construct(&fenv, id_tok, env); // TODO: function name
        // fprint_impl(fp, node->value.func_def.decl_spec, 0);
        // fprint_impl(fp, node->value.func_def.decl, 0);
        analyze(a, node->value.func_def.block, env);

        env_insert(env, &fenv);

        break;
    }

    case NODE_STMT_COMPOUND:
    {
        printf("LOG: statement compound\n");

        Vector* stmts = node->value.stmt_compound.stmts;
        for(size_t i=0; i<vector_len(stmts); ++i) {
            Node** n = (Node**)vector_at(stmts, i);
            analyze(a, *n, env);
        }
        break;
    }

    case NODE_STMT_EXPR:
        if (node->value.stmt_expr.expr) {
            analyze_expr(a, node->value.stmt_expr.expr, env);
        }
        break;

    case NODE_STMT_IF:
        // TODO: Dig a scope
        analyze_expr(a, node->value.stmt_if.cond, env);

        analyze(a, node->value.stmt_if.then_b, env);

        if (node->value.stmt_if.else_b) {
            analyze(a, node->value.stmt_if.else_b, env);
        }
        break;

    case NODE_STMT_JUMP:
        printf("LOG: statement jump\n");

        switch(node->value.stmt_jump.kind) {
        case TOK_KIND_RETURN:
            analyze_expr(a, node->value.stmt_jump.expr, env);
            break;

        default:
            // TODO: error handling...
            assert(0);
            break;
        }
        break;

    default:
        // TODO: error handling...
        fprintf(stderr, "Unknown kind: %d\n", node->kind);
        assert(0);
        break;
    }
}

void analyze_expr(Analyzer* a, Node* node, Env* env) {
    switch(node->kind) {
    case NODE_EXPR_BIN:
    {
        printf("LOG: expr binary = ");
        token_fprint_buf(stdout, node->value.expr_bin.op);
        printf("\n");

        analyze_expr(a, node->value.expr_bin.lhs, env);
        analyze_expr(a, node->value.expr_bin.rhs, env);

        break;
    }

    case NODE_EXPR_POSTFIX:
    {
        printf("LOG: postfix = ");

        break;
    }

    case NODE_LIT_INT:
    {
        printf("LOG: lit int = %d\n", node->value.lit_int.v);

        break;
    }

    case NODE_ID:
    {
        printf("LOG: id =");
        token_fprint_buf(stdout, node->value.id.tok);
        printf("\n");

        break;
    }

    default:
        // TODO: error handling...
        fprintf(stderr, "Unknown kind: %d", node->kind);
        assert(0);
        break;
    }
}

int analyzer_success(Analyzer* a) {
    return 1; // TODO: implement
}
