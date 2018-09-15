#include "node.h"

static void fprint_impl(FILE *fp, Node* node, int indent);
static void fprint_indent(FILE *fp, int indent);

void node_drop(Node* node) {
    switch(node->kind) {
    case NODE_STMT_COMPOUND:
        vector_drop(node->value.stmt_compound.stmts);
        break;

    default:
        break;
    }
}

void node_fprint(FILE *fp, Node* node) {
    fprint_impl(fp, node, 0);
}

void fprint_impl(FILE *fp, Node* node, int indent) {
    switch(node->kind) {
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
        for(int i=0; i<vector_len(stmts); ++i) {
            Node** n = (Node**)vector_at(stmts, i);
            fprint_impl(fp, *n, indent + 1);
        }
        fprint_indent(fp, indent); fprintf(fp, "}\n");
        break;
    }

    case NODE_STMT_JUMP:
        switch(node->value.stmt_jump.kind) {
        case TOK_KIND_RETURN:
            fprint_indent(fp, indent); fprintf(fp, "return ");
            fprint_impl(fp, node->value.stmt_jump.expr, 0);
            fprintf(fp, " ;\n");
            break;

        default:
            // TODO: error handling...
            break;
        }
        break;

    case NODE_LIT_INT:
        fprintf(fp, "%d", node->value.lit_int.v);
        break;

    case NODE_ID:
        token_fprint_buf(fp, node->value.id.tok);
        break;

    default:
        // TODO: error handling...
        break;
    }
}

void fprint_indent(FILE *fp, int indent) {
    for(int i=0; i<indent*4; ++i) {
        fprintf(fp, " ");
    }
}
