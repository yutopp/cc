#include <stdlib.h>
#include <assert.h>
#include "ir.h"
#include "ir_inst_defs.h"
#include "vector.h"
#include "map.h"
#include "log.h"

static void ir_bb_append_inst(IRBB* bb, IRInst* inst) {
    IRInst* i = vector_append(bb->insts);
    *i = *inst;
}

static void ir_bb_terminate(IRBB* bb, IRInst inst) {
    assert(bb->term == NULL);
    // Set terminal
    bb->term = (IRInst*)malloc(sizeof(IRInst));
    *bb->term = inst;
}

static void ir_function_coustruct(IRFunction* f, char const* name, IRModule* m) {
    f->name = name;
    f->mod = m;
    f->bb_arena = ir_bb_arena_new();
    f->entry = ir_bb_arena_malloc(f->bb_arena);
    f->bb_id = 0;
    f->locals = vector_new(sizeof(size_t));
    f->locals_id = 0;

    ir_bb_construct(f->entry, f->bb_id);
    f->bb_id++;
}

static void ir_function_destruct(IRFunction* f) {
    free((char*)f->name);
    ir_bb_arena_drop(f->bb_arena);
    vector_drop(f->locals);
}

void ir_function_set_local(IRFunction* f, IRSymbolID id, size_t size) {
    while(vector_len(f->locals) <= id) {
        void* e = vector_append(f->locals);
        assert(e);
    }

    size_t* s = vector_at(f->locals, id);
    *s = size;
}

size_t ir_function_get_local(IRFunction* f, IRSymbolID id) {
    size_t* s = vector_at(f->locals, id);
    return *s;
}

IRModule* ir_module_new() {
    IRModule* m = (IRModule*)malloc(sizeof(IRModule));
    m->definitions = vector_new(sizeof(IRInst));
    m->functions = vector_new(sizeof(IRFunction));
    m->defs_sym_id = 0;

    return m;
}

void ir_module_drop(IRModule *m) {
    for(size_t i=0; i<vector_len(m->definitions); ++i) {
        IRInst* inst = vector_at(m->definitions, i);
        ir_inst_destruct(inst);
    }
    vector_drop(m->definitions);

    for(size_t i=0; i<vector_len(m->functions); ++i) {
        IRFunction* f = vector_at(m->functions, i);
        ir_function_destruct(f);
    }
    vector_drop(m->functions);

    free(m);
}

static IRSymbolID insert_definition(IRModule* m, IRInstValue v) {
    IRSymbolID sym_id = m->defs_sym_id;
    m->defs_sym_id++;

    IRInst inst = {
        .kind = IR_INST_KIND_LET,
        .value = {
            .let = {
                .id = sym_id,
                .rhs = v,
            },
        },
    };
    IRInst* i = vector_append(m->definitions);
    *i = inst;

    return sym_id;
}

static void build_trans_unit(IRBuilder* builder, Node* node, IRModule* m);
static void build_top_level(IRBuilder* builder, Node* node, IRModule* m);
static void build_statement(IRBuilder* builder, Node* node, IRFunction* m);
static IRSymbolID build_expression(IRBuilder* builder, Node* node, IRFunction* f);

struct ir_builder_t {
    IRFunction* current_func;
    IRBB* current_bb;
};

IRBuilder* ir_builder_new() {
    IRBuilder* builder = (IRBuilder*)malloc(sizeof(IRBuilder));
    builder->current_func = NULL;
    builder->current_bb = NULL;

    return builder;
}

void ir_builder_drop(IRBuilder* builder) {
    free(builder);
}

IRModule* ir_builder_new_module(IRBuilder* builder, Node* node) {
    IRModule* m = ir_module_new();

    build_trans_unit(builder, node, m);

    return m;
}

static IRFunction* ir_builder_build_function(IRBuilder* builder, char const* name, IRModule* m) {
    IRFunction* f = vector_append(m->functions);
    ir_function_coustruct(f, name, m);

    return f;
}

static void ir_builder_set_current_func(IRBuilder* builder, IRFunction* f) {
    builder->current_func = f;
    builder->current_bb = f->entry;
}

static void ir_builder_set_current_bb(IRBuilder* builder, IRBB* bb) {
    builder->current_bb = bb;
}

static IRBB* ir_builder_build_bb(IRBuilder* builder) {
    IRFunction* f = builder->current_func;
    IRBB* bb = ir_bb_arena_malloc(f->bb_arena);
    ir_bb_construct(bb, f->bb_id);
    f->bb_id++;

    return bb;
}

static IRSymbolID ir_builder_build_local(IRBuilder* builder) {
    IRFunction* f = builder->current_func;
    IRSymbolID sym_id = f->locals_id;
    f->locals_id++;

    return sym_id;
}

void build_trans_unit(IRBuilder* builder, Node* node, IRModule* m) {
    switch(node->kind) {
    case NODE_TRANS_UNIT:
    {
        printf("LOG: transition unit\n");

        Vector* decls = node->value.trans_unit.decls;

        for(size_t i=0; i<vector_len(decls); ++i) {
            Node** n = (Node**)vector_at(decls, i);
            build_top_level(builder, *n, m);
        }

        break;
    }

    default:
        fprintf(stderr, "Kind = %d\n", node->kind);
        assert(0); // TODO: error handling
    }
}

void build_top_level(IRBuilder* builder, Node* node, IRModule* m) {
    switch(node->kind) {
    case NODE_FUNC_DEF:
    {
        printf("LOG: function def\n");

        Token* id_tok = node_declarator_extract_id_token(node->value.func_def.decl);
        assert(id_tok);

        IRInstValue fval = {
            .kind = IR_INST_VALUE_KIND_SYMBOL,
            .value = {
                .symbol = token_to_string(id_tok),
            },
        };
        insert_definition(m, fval);

        IRFunction* f = ir_builder_build_function(builder, token_to_string(id_tok), m);
        ir_builder_set_current_func(builder, f);
        build_statement(builder, node->value.func_def.block, f);

        break;
    }

    default:
        fprintf(stderr, "Kind = %d\n", node->kind);
        assert(0); // TODO: error handling
    }
}

void build_statement(IRBuilder* builder, Node* node, IRFunction* f) {
    switch(node->kind) {
    case NODE_STMT_COMPOUND:
    {
        printf("LOG: statement compound\n");

        Vector* stmts = node->value.stmt_compound.stmts;
        for(size_t i=0; i<vector_len(stmts); ++i) {
            Node** n = (Node**)vector_at(stmts, i);
            build_statement(builder, *n, f);
        }
        break;
    }

    case NODE_STMT_EXPR:
        printf("LOG: statement expr\n");

        if (node->value.stmt_expr.expr) {
            build_expression(builder, node->value.stmt_expr.expr, f);
        }
        break;

    case NODE_STMT_IF:
        fprintf(DEBUGOUT, "LOG: statement if\n");

        IRSymbolID cond = build_expression(builder, node->value.stmt_if.cond, f);

        IRBB* then_bb = ir_builder_build_bb(builder);
        IRBB* else_bb = NULL;
        if (node->value.stmt_if.else_b) {
            else_bb = ir_builder_build_bb(builder);
        }
        IRBB* final_bb = ir_builder_build_bb(builder);

        // Terminate current bb
        IRInst inst = {
            .kind = IR_INST_KIND_BRANCH,
            .value = {
                .branch = {
                    .cond = cond,
                    .then_bb = then_bb,
                    .else_bb = else_bb != NULL ? else_bb : final_bb,
                },
            },
        };
        ir_bb_terminate(builder->current_bb, inst);

        // then block
        ir_builder_set_current_bb(builder, then_bb);
        build_statement(builder, node->value.stmt_if.then_b, f);
        if (builder->current_bb->term == NULL) {
            IRInst then_inst = {
                .kind = IR_INST_KIND_JUMP,
                .value = {
                    .jump = {
                        .next_bb = final_bb,
                    },
                },
            };
            ir_bb_terminate(builder->current_bb, then_inst);
        }

        // then block
        if (node->value.stmt_if.else_b) {
            ir_builder_set_current_bb(builder, else_bb);
            build_statement(builder, node->value.stmt_if.else_b, f);
            if (builder->current_bb->term == NULL) {
                IRInst else_inst = {
                    .kind = IR_INST_KIND_JUMP,
                    .value = {
                        .jump = {
                            .next_bb = final_bb,
                        },
                    },
                };
                ir_bb_terminate(builder->current_bb, else_inst);
            }
        }

        ir_builder_set_current_bb(builder, final_bb);

        break;

    case NODE_STMT_JUMP:
        printf("LOG: statement jump\n");

        switch(node->value.stmt_jump.kind) {
        case TOK_KIND_RETURN:
        {
            IRSymbolID expr_ref = build_expression(builder, node->value.stmt_jump.expr, f);

            IRInst inst = {
                .kind = IR_INST_KIND_RET,
                .value = {
                    .ret = {
                        .id = expr_ref,
                    },
                },
            };
            ir_bb_terminate(builder->current_bb, inst);

            break;
        }

        default:
            assert(0); // TODO: error handling...
            break;
        }
        break;

    default:
        assert(0); // TODO: error handling...
        break;
    }
}

IRSymbolID build_expression(IRBuilder* builder, Node* node, IRFunction* f) {
    switch(node->kind) {
    case NODE_EXPR_BIN:
    {
        printf("LOG: expr bin = ");
        token_fprint_buf(stdout, node->value.expr_bin.op);
        printf("\n");

        IRSymbolID lhs_sym = build_expression(builder, node->value.expr_bin.lhs, f);
        IRSymbolID rhs_sym = build_expression(builder, node->value.expr_bin.rhs, f);

        IRSymbolID sym_id = ir_builder_build_local(builder);

        IRInstValue bin = {
            .kind = IR_INST_VALUE_KIND_OP_BIN,
            .value = {
                .op_bin = {
                    .op = node->value.expr_bin.op, // TODO: Change to builtin enums...
                    .lhs = lhs_sym,
                    .rhs = rhs_sym,
                },
            },
        };
        IRInst inst = {
            .kind = IR_INST_KIND_LET,
            .value = {
                .let = {
                    .id = sym_id,
                    .rhs = bin,
                },
            },
        };
        ir_bb_append_inst(builder->current_bb, &inst);
        ir_function_set_local(f, sym_id, 8); // TODO: fix

        return sym_id;
    }

    case NODE_EXPR_POSTFIX:
    {
        printf("LOG: expr post = \n");

        IRSymbolID lhs_sym = build_expression(builder, node->value.expr_postfix.lhs, f);

        switch(node->value.expr_postfix.kind) {
        case NODE_EXPR_POSTFIX_KIND_FUNC_CALL:
        {
            Vector* args = vector_new(sizeof(IRSymbolID)); // Vector<IRSymbolID>
            Node* args_list = node->value.expr_postfix.rhs;
            if (args_list) {
                assert(args_list->kind == NODE_ARGS_LIST);
                for(size_t i=0; i<vector_len(args_list->value.args_list.args); ++i) {
                    Node** a = (Node**)vector_at(args_list->value.args_list.args, i);
                    IRSymbolID a_sym = build_expression(builder, *a, f);

                    IRSymbolID* args_e = (IRSymbolID*)vector_append(args);
                    *args_e = a_sym;
                }
            }

            IRSymbolID sym_id = ir_builder_build_local(builder);

            IRInstValue call = {
                .kind = IR_INST_VALUE_KIND_CALL,
                .value = {
                    .call = {
                        .lhs = lhs_sym,
                        .args = args,
                    },
                },
            };
            IRInst inst = {
                .kind = IR_INST_KIND_LET,
                .value = {
                    .let = {
                        .id = sym_id,
                        .rhs = call,
                    },
                },
            };
            ir_bb_append_inst(builder->current_bb, &inst);
            ir_function_set_local(f, sym_id, 8); // TODO: fix

            return sym_id;
        }

        default:
            assert(0); // TODO: error handling...
        }
        break;
    }

    case NODE_LIT_INT:
    {
        printf("LOG: lit_int\n");
        IRSymbolID sym_id = ir_builder_build_local(builder);

        IRInstValue imm = {
            .kind = IR_INST_VALUE_KIND_IMM_INT,
            .value = {
                .imm_int = node->value.lit_int.v,
            },
        };
        IRInst inst = {
            .kind = IR_INST_KIND_LET,
            .value = {
                .let = {
                    .id = sym_id,
                    .rhs = imm,
                },
            },
        };
        ir_bb_append_inst(builder->current_bb, &inst);
        ir_function_set_local(f, sym_id, 8); // TODO: fix

        return sym_id;
    }

    case NODE_LIT_STRING:
    {
        printf("LOG: lit_string\n");

        IRInstValue sval = {
            .kind = IR_INST_VALUE_KIND_STRING,
            .value = {
                .string = node->value.lit_string.v,
            },
        };
        IRSymbolID str_sym_id = insert_definition(f->mod, sval);

        //
        IRSymbolID ref_sym_id = ir_builder_build_local(builder);

        {
            IRInstValue ref = {
                .kind = IR_INST_VALUE_KIND_REF,
                .value = {
                    .ref = {
                        .is_global = 1,
                        .sym = str_sym_id,
                    },
                },
            };
            IRInst inst = {
                .kind = IR_INST_KIND_LET,
                .value = {
                    .let = {
                        .id = ref_sym_id,
                        .rhs = ref,
                    },
                },
            };
            ir_bb_append_inst(builder->current_bb, &inst);
            ir_function_set_local(f, ref_sym_id, 8); // TODO: fix
        }

        IRSymbolID sym_id = ir_builder_build_local(builder);

        IRInstValue addr_of = {
            .kind = IR_INST_VALUE_KIND_ADDR_OF,
            .value = {
                .addr_of = {
                    .sym = ref_sym_id,
                },
            },
        };
        IRInst inst = {
            .kind = IR_INST_KIND_LET,
            .value = {
                .let = {
                    .id = sym_id,
                    .rhs = addr_of,
                },
            },
        };
        ir_bb_append_inst(builder->current_bb, &inst);
        ir_function_set_local(f, sym_id, 8); // TODO: fix

        return sym_id;
    }

    case NODE_ID:
    {
        // TODO: implement
        IRInstValue val = {
            .kind = IR_INST_VALUE_KIND_SYMBOL,
            .value = {
                .symbol = token_to_string(node->value.id.tok),
            },
        };
        IRSymbolID tmp_sym_id = insert_definition(f->mod, val);

        IRSymbolID sym_id = ir_builder_build_local(builder);

        IRInstValue ref = {
            .kind = IR_INST_VALUE_KIND_REF,
            .value = {
                .ref = {
                    .is_global = 1,
                    .sym = tmp_sym_id,
                },
            },
        };
        IRInst inst = {
            .kind = IR_INST_KIND_LET,
            .value = {
                .let = {
                    .id = sym_id,
                    .rhs = ref,
                },
            },
        };
        ir_bb_append_inst(builder->current_bb, &inst);
        ir_function_set_local(f, sym_id, 8); // TODO: fix

        return sym_id;
    }

    default:
        assert(0); // TODO: error handling...
    }
}

static void fprint_indent(FILE *fp, int indent);

void ir_module_fprint(FILE* fp, IRModule* m) {
    for(size_t i=0; i<vector_len(m->definitions); ++i) {
        IRInst* inst = vector_at(m->definitions, i);
        ir_inst_fprint(fp, inst);
    }

    for(size_t i=0; i<vector_len(m->functions); ++i) {
        IRFunction* f = vector_at(m->functions, i);
        ir_function_fprint(fp, f);
    }
}

static void ir_function_fprint_iter(IRBB* bb, void* args) {
    FILE* fp = (FILE*)args;
    ir_bb_fprint(fp, bb);
}

void ir_function_fprint(FILE* fp, IRFunction* f) {
    fprint_indent(fp, 0); fprintf(fp, "%s:\n", f->name);
    ir_bb_visit(f->entry, ir_function_fprint_iter, fp);
}

void ir_bb_fprint(FILE* fp, IRBB* bb) {
    fprint_indent(fp, 1); fprintf(fp, "%ld:\n", bb->id);

    for(size_t i=0; i<vector_len(bb->insts); ++i) {
        IRInst* inst = vector_at(bb->insts, i);
        fprint_indent(fp, 2); ir_inst_fprint(fp, inst);
    }

    if (bb->term != NULL) {
        fprint_indent(fp, 2); ir_inst_fprint(fp, bb->term);
    } else {
        fprint_indent(fp, 2); fprintf(fp, "NOT TERMINATED\n");
    }
}

void ir_inst_fprint(FILE* fp, IRInst* inst) {
    switch(inst->kind) {
    case IR_INST_KIND_LET:
        fprintf(fp, "%%%ld = ", inst->value.let.id);
        switch(inst->value.let.rhs.kind) {
        case IR_INST_VALUE_KIND_SYMBOL:
        {
            fprintf(fp, "SYMBOL %s", inst->value.let.rhs.value.symbol);
            break;
        }

        case IR_INST_VALUE_KIND_STRING:
        {
            fprintf(fp, "\"%s\"", inst->value.let.rhs.value.string);
            break;
        }

        case IR_INST_VALUE_KIND_REF:
        {
            if (inst->value.let.rhs.value.ref.is_global) {
                fprintf(fp, "g@");
            }
            fprintf(fp, "%ld", inst->value.let.rhs.value.ref.sym);
            break;
        }

        case IR_INST_VALUE_KIND_ADDR_OF:
        {
            fprintf(fp, "&%ld", inst->value.let.rhs.value.addr_of.sym);
            break;
        }

        case IR_INST_VALUE_KIND_IMM_INT:
        {
            fprintf(fp, "%d", inst->value.let.rhs.value.imm_int);
            break;
        }

        case IR_INST_VALUE_KIND_OP_BIN:
        {
            char const* op_buf = token_to_string(inst->value.let.rhs.value.op_bin.op);
            fprintf(fp, "%s %%%ld %%%ld",
                    op_buf,
                    inst->value.let.rhs.value.op_bin.lhs,
                    inst->value.let.rhs.value.op_bin.rhs);
            free((char*)op_buf);
            break;
        }

        case IR_INST_VALUE_KIND_CALL:
        {
            fprintf(fp, "call %%%ld", inst->value.let.rhs.value.call.lhs);
            Vector* args = inst->value.let.rhs.value.call.args;
            for(size_t i=0; i<vector_len(args); ++i) {
                IRSymbolID* sym = (IRSymbolID*)vector_at(args, i);
                fprintf(fp, " %%%ld", *sym);
            }
            break;
        }

        default:
            assert(0); // TODO: error handling
        }
        fprintf(fp, "\n");
        break;

    case IR_INST_KIND_RET:
        fprintf(fp, "RET %%%ld\n", inst->value.ret.id);
        break;

    case IR_INST_KIND_BRANCH:
        fprintf(fp, "BR %%%ld: ", inst->value.branch.cond);
        fprintf(fp, "then -> %ld, ", inst->value.branch.then_bb->id);
        fprintf(fp, "else -> %ld\n", inst->value.branch.else_bb->id);

        break;

    case IR_INST_KIND_JUMP:
        fprintf(fp, "JUMP -> %ld\n", inst->value.jump.next_bb->id);
        break;

    default:
        assert(0); // TODO: error handling
    }
}

void fprint_indent(FILE *fp, int indent) {
    for(int i=0; i<indent*2; ++i) {
        fprintf(fp, " ");
    }
}
