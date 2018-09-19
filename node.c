#include <stdlib.h>
#include <assert.h>
#include "node.h"

static void fprint_impl(FILE *fp, Node* node, int indent);
static void fprint_indent(FILE *fp, int indent);

void node_destruct(Node* node) {
    switch(node->kind) {
    case NODE_TRANS_UNIT:
        vector_drop(node->value.trans_unit.decls);
        break;

    case NODE_STMT_COMPOUND:
        vector_drop(node->value.stmt_compound.stmts);
        break;

    case NODE_LIT_STRING:
        free((char*)node->value.lit_string.v);
        break;

    case NODE_ARGS_LIST:
        vector_drop(node->value.args_list.args);
        break;

    case NODE_PARAM_LIST:
        vector_drop(node->value.param_list.params);
        break;

    default:
        // DO NOTHING
        break;
    }
}

void node_fprint(FILE *fp, Node* node) {
    fprint_impl(fp, node, 0);
}

Token* node_declarator_extract_id_token(Node* node) {
    switch(node->kind) {
    case NODE_DECLARATOR:
        return node_declarator_extract_id_token(node->value.declarator.node);
    case NODE_DIRECT_DECLARATOR:
        return node_declarator_extract_id_token(node->value.direct_declarator.base);
    case NODE_ID:
        return node->value.id.tok;
    default:
        return NULL;
    }
}

void fprint_impl(FILE *fp, Node* node, int indent) {
    switch(node->kind) {
    case NODE_TRANS_UNIT:
    {
        Vector* decls = node->value.trans_unit.decls;
        for(size_t i=0; i<vector_len(decls); ++i) {
            Node** n = (Node**)vector_at(decls, i);
            fprint_impl(fp, *n, indent);
        }
        break;
    }

    case NODE_FUNC_DEF:
        fprint_indent(fp, indent); fprint_impl(fp, node->value.func_def.decl_spec, 0);
        fprintf(fp, " ");
        fprint_indent(fp, indent); fprint_impl(fp, node->value.func_def.decl, 0);
        fprint_indent(fp, indent); fprintf(fp, "\n");
        fprint_impl(fp, node->value.func_def.block, indent);
        break;

    case NODE_STMT_COMPOUND:
    {
        fprint_indent(fp, indent); fprintf(fp, "{\n");
        Vector* stmts = node->value.stmt_compound.stmts;
        for(size_t i=0; i<vector_len(stmts); ++i) {
            Node** n = (Node**)vector_at(stmts, i);
            fprint_impl(fp, *n, indent + 1);
        }
        fprint_indent(fp, indent); fprintf(fp, "}\n");
        break;
    }

    case NODE_STMT_EXPR:
        if (node->value.stmt_expr.expr) {
            fprint_indent(fp, indent); fprint_impl(fp, node->value.stmt_expr.expr, 0);
        }
        fprintf(fp, " ;\n");
        break;

    case NODE_STMT_JUMP:
        switch(node->value.stmt_jump.kind) {
        case TOK_KIND_RETURN:
            fprint_indent(fp, indent); fprintf(fp, "return ");
            fprint_impl(fp, node->value.stmt_jump.expr, 0);
            fprintf(fp, " ;\n");
            break;

        default:
            assert(0); // TODO: error handling...
            break;
        }
        break;

    case NODE_EXPR_BIN:
        fprint_impl(fp, node->value.expr_bin.lhs, indent);
        token_fprint_buf(fp, node->value.expr_bin.op);
        fprint_impl(fp, node->value.expr_bin.rhs, indent);
        break;

    case NODE_EXPR_POSTFIX:
        switch(node->value.expr_postfix.kind) {
        case NODE_EXPR_POSTFIX_KIND_FUNC_CALL:
            fprint_impl(fp, node->value.expr_postfix.lhs, indent);
            fprintf(fp, "(");
            if (node->value.expr_postfix.rhs) {
                fprint_impl(fp, node->value.expr_postfix.rhs, indent);
            }
            fprintf(fp, ")");
            break;

        default:
            assert(0); // TODO: error handling...
        }
        break;

    case NODE_LIT_INT:
        fprintf(fp, "%d", node->value.lit_int.v);
        break;

    case NODE_LIT_STRING:
        fprintf(fp, "\"%s\"", node->value.lit_string.v);
        break;

    case NODE_ID:
        token_fprint_buf(fp, node->value.id.tok);
        break;

    case NODE_ARGS_LIST:
    {
        Vector* args = node->value.args_list.args;
        for(size_t i=0; i<vector_len(args); ++i) {
            if (i != 0) {
                fprintf(fp, ", ");
            }
            Node** n = (Node**)vector_at(args, i);
            fprint_impl(fp, *n, 0);
        }
        break;
    }

    case NODE_DECLARATOR:
        fprint_impl(fp, node->value.declarator.node, 0);
        break;

    case NODE_DIRECT_DECLARATOR:
        fprint_impl(fp, node->value.direct_declarator.base, 0);
        switch(node->value.direct_declarator.kind) {
        case NODE_DIRECT_DECLARATOR_KIND_BASE:
            break; // DO NOTHING

        case NODE_DIRECT_DECLARATOR_KIND_PARAM_TYPE_LIST:
            fprintf(fp, "(");
            fprint_impl(fp, node->value.direct_declarator.args[0], 0);
            fprintf(fp, ")");
            break;

        default:
            assert(0); // TODO: error handling...
        }
        break;

    case NODE_PARAM_DECL:
        fprint_impl(fp, node->value.param_decl.spec, 0);
        if (node->value.param_decl.decl) {
            fprintf(fp, " ");
            fprint_impl(fp, node->value.param_decl.decl, 0);
        }
        break;

    case NODE_PARAM_LIST:
    {
        Vector* params = node->value.param_list.params;
        for(size_t i=0; i<vector_len(params); ++i) {
            if (i != 0) {
                fprintf(fp, ", ");
            }
            Node** n = (Node**)vector_at(params, i);
            fprint_impl(fp, *n, 0);
        }
        break;
    }

    default:
        fprintf(stderr, "Unknown kind: %d", node->kind);
        assert(0); // TODO: error handling...
        break;
    }
}

void fprint_indent(FILE *fp, int indent) {
    for(int i=0; i<indent*4; ++i) {
        fprintf(fp, " ");
    }
}
