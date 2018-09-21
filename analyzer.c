#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "analyzer.h"
#include "map.h"
#include "log.h"

typedef enum {
    ENV_KIND_TRANS,
} EnvKind;

struct env_t;
typedef struct env_t Env;

struct env_t {
    Env* parent; // Nullable
    Token* name_tok;
    char const* name;
    EnvKind kind;
    union {
    } value;
    StringMap* children; // Map<char const*, Env*>
};

Env* env_new(Token* name, Env* parent);
void env_drop(Env* env);

static void env_elem_dtor(void* env_ref) {
    env_drop(*(Env**)env_ref);
}

Env* env_new(Token* name_tok, Env* parent) {
    Env* e = (Env*)malloc(sizeof(Env));
    e->parent = parent;
    e->name_tok = name_tok;
    e->name = e->name_tok ? token_to_string(e->name_tok) : NULL;
    e->children = string_map_new(sizeof(Env*), env_elem_dtor);

    return e;
}

void env_drop(Env* env) {
    if (env->name) {
        free((char*)env->name);
    }
    string_map_drop(env->children);
    free(env);
}

Env* env_insert(Env* env, Env* child) {
    int found;
    assert(child->name);

    Env** e = string_map_insert(env->children, child->name, &found);
    assert(found == 0);
    *e = child;

    return *e;
}

Env* env_lookup(Env* env, Token* name_tok) {
    char const* name = token_to_string(name_tok);
    Env* found_env = string_map_find(env->children, name);
    free((char*)name);

    if (found_env) {
        return found_env;
    }

    if (env->parent == NULL) {
        return NULL;
    }

    return env_lookup(env->parent, name_tok);
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
    analyze(a, node, NULL);
}

void analyze(Analyzer* a, Node* node, Env* env) {
    switch(node->kind) {
    case NODE_TRANS_UNIT:
    {
        printf("LOG: translation unit\n");
        Env* t_env = env_new(NULL, env); // TODO: Set a name of the translation unit
        t_env->kind = ENV_KIND_TRANS;

        Vector* decls = node->value.trans_unit.decls;
        for(size_t i=0; i<vector_len(decls); ++i) {
            Node** n = (Node**)vector_at(decls, i);
            analyze(a, *n, t_env);
        }

        env_drop(t_env);
        break;
    }

    case NODE_FUNC_DEF:
    {
        printf("LOG: function def\n");

        node_fprint(stdout, node->value.func_def.decl);
        assert(node->value.func_def.decl->kind == NODE_DECLARATOR);
        Token* id_tok = node_declarator_extract_id_token(node->value.func_def.decl);
        assert(id_tok); // TODO: error handling

        // TODO: Lookup a decl

        Env* f_env = env_new(id_tok, env);
        f_env->kind = ENV_KIND_TRANS;

        // fprint_impl(fp, node->value.func_def.decl_spec, 0);
        // fprint_impl(fp, node->value.func_def.decl, 0);
        analyze(a, node->value.func_def.block, f_env);

        env_insert(env, f_env);

        break;
    }

    case NODE_STMT_COMPOUND:
    {
        printf("LOG: statement compound\n");

        Env* b_env = env_new(NULL, env);
        b_env->kind = ENV_KIND_TRANS;

        Vector* stmts = node->value.stmt_compound.stmts;
        for(size_t i=0; i<vector_len(stmts); ++i) {
            Node** n = (Node**)vector_at(stmts, i);
            analyze(a, *n, b_env);
        }

        env_drop(b_env);
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
        fprintf(DEBUGOUT,"LOG: postfix = \n");

        analyze_expr(a, node->value.expr_postfix.lhs, env);

        switch(node->value.expr_postfix.kind) {
        case NODE_EXPR_POSTFIX_KIND_FUNC_CALL:
            if (node->value.expr_postfix.rhs) {
                analyze_expr(a, node->value.expr_postfix.rhs, env);
            }
            break;

        default:
            assert(0); // TODO: error handling
        }

        break;
    }

    case NODE_LIT_INT:
    {
        printf("LOG: lit int = %d\n", node->value.lit_int.v);

        break;
    }

    case NODE_ID:
    {
        fprintf(DEBUGOUT, "LOG: id =");
        token_fprint_buf(DEBUGOUT, node->value.id.tok);
        fprintf(DEBUGOUT, "\n");

        Env* found = env_lookup(env, node->value.id.tok);
        if (found == NULL) {
            fprintf(DEBUGOUT, "! NOT FOUND\n");
        }
        fprintf(DEBUGOUT, "! FOUND\n");

        break;
    }

    case NODE_ARGS_LIST:
    {
        fprintf(DEBUGOUT, "LOG: args list\n");

        Vector* args = node->value.args_list.args;
        for(size_t i=0; i<vector_len(args); ++i) {
            Node** n = (Node**)vector_at(args, i);
            analyze_expr(a, *n, env);
        }

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
