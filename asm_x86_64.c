#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include "asm_x86_64.h"
#include "ir.h"
#include "ir_inst_defs.h"
#include "vector.h"
#include "map.h"
#include "log.h"

static void built_from_ir(ASM_X86_64 *a, IRModule* m);
static void built_from_ir_function(ASM_X86_64 *a, IRFunction* f);
static void collect_labels_from_ir_bb(ASM_X86_64 *a, IRBB* bb);
static void built_from_ir_bb(ASM_X86_64 *a, IRBB* bb);
static void built_from_ir_inst(ASM_X86_64 *a, IRInst* inst);
static void built_from_ir_global_inst(ASM_X86_64 *a, IRInst* inst);

struct asm_x86_64_t {
    Vector* insts;         // Vector<ASM_X86_64_Inst>
    Vector* values;        // Vector<ASM_X86_64_Var>
    Vector* global_values; // Vector<ASM_X86_64_Var>
    Vector* offsets;       // Vector<size_t>
    size_t string_label_count;
    size_t code_label_count;
    UintMap* labels;
};

typedef enum asm_x86_64_op_t {
    ASM_X86_64_OP_PUSHQ, // v
    ASM_X86_64_OP_POPQ,  // v
    ASM_X86_64_OP_MOVQ,  // d, s
    ASM_X86_64_OP_MOVL,  // d, s
    ASM_X86_64_OP_ADDQ,  // d, s
    ASM_X86_64_OP_SUBQ,  // d, s
    ASM_X86_64_OP_LEAQ,  // d, s
    ASM_X86_64_OP_CALL,  // v
    ASM_X86_64_OP_CMPQ,  // d, s
    ASM_X86_64_OP_JMP,   // v
    ASM_X86_64_OP_JE,    // v
    ASM_X86_64_OP_RET,   // (none)
} ASM_X86_64_Op;

typedef enum asm_x86_64_reg_t {
    ASM_X86_64_REG_RAX,
    ASM_X86_64_REG_RBX,
    ASM_X86_64_REG_RCX,
    ASM_X86_64_REG_RDX,
    ASM_X86_64_REG_RSP,
    ASM_X86_64_REG_RBP,
    ASM_X86_64_REG_RSI,
    ASM_X86_64_REG_RDI,
    ASM_X86_64_REG_RIP,

    ASM_X86_64_REG_EAX,
} ASM_X86_64_Reg;

static ASM_X86_64_Reg arg_regs[4] = {
    ASM_X86_64_REG_RDI,
    ASM_X86_64_REG_RSI,
    ASM_X86_64_REG_RDX,
    ASM_X86_64_REG_RCX,
};

typedef enum asm_x86_64_value_kind_t {
    ASM_X86_64_VALUE_KIND_SYMBOL,
    ASM_X86_64_VALUE_KIND_IMM_INT,
    ASM_X86_64_VALUE_KIND_STRING,
    ASM_X86_64_VALUE_KIND_REG,
    ASM_X86_64_VALUE_KIND_DISP_REG,
} ASM_X86_64_ValueKind;

struct asm_x86_64_value_t;
typedef struct asm_x86_64_value_t ASM_X86_64_Value;

struct asm_x86_64_value_t {
    ASM_X86_64_ValueKind kind;
    union {
        char const* symbol;
        int imm_int;
        struct {
            char const* label;
        } string;
        ASM_X86_64_Reg reg;
        struct {
            char const* symbol;
            int disp; // 32bits
            ASM_X86_64_Reg reg;
        } disp_reg;
    } value;
};

typedef enum asm_x86_64_var_kind_t {
    ASM_X86_64_VAR_REF,
    ASM_X86_64_VAR_VAL,
} ASM_X86_64_VarKind;

typedef struct asm_x86_64_var_t {
    ASM_X86_64_VarKind kind;
    union {
        IRSymbolID ref;
        ASM_X86_64_Value val;
    } value;
} ASM_X86_64_Var;

typedef enum asm_x86_64_inst_kind_t {
    ASM_X86_64_INST_KIND_SECTION,
    ASM_X86_64_INST_KIND_STRING,
    ASM_X86_64_INST_KIND_TEXT,
    ASM_X86_64_INST_KIND_GLOBAL,
    ASM_X86_64_INST_KIND_TYPE,
    ASM_X86_64_INST_KIND_LABEL,
    ASM_X86_64_INST_KIND_OP,
} ASM_X86_64_InstKind;

struct asm_x86_64_inst_t {
    ASM_X86_64_InstKind kind;
    union {
        struct {
            char const* name;
        } section;
        struct {
            char const* s;
        } string;
        struct {
            char const* name;
        } global;
        struct {
            char const* name;
            char const* type;
        } type;
        struct {
            char const* name;
            int generated;
        } label;
        struct {
            ASM_X86_64_Op op;
            ASM_X86_64_Value args[4];
        } op;
    } value;
};

static void asm_x86_64_inst_destruct(ASM_X86_64_Inst* inst) {
    switch(inst->kind) {
    case ASM_X86_64_INST_KIND_LABEL:
        if (inst->value.label.generated) {
            free((char*)inst->value.label.name);
        }
        break;

    default:
        // DO NOTHING
        break;
    }
}

ASM_X86_64* asm_x86_64_new(IRModule* m) {
    ASM_X86_64* a = (ASM_X86_64*)malloc(sizeof(ASM_X86_64));
    a->insts = vector_new(sizeof(ASM_X86_64_Inst));
    a->values = vector_new(sizeof(ASM_X86_64_Var));
    a->global_values = vector_new(sizeof(ASM_X86_64_Var));
    a->offsets = vector_new(sizeof(size_t));
    a->string_label_count = 0;
    a->code_label_count = 0;
    a->labels = uint_map_new(sizeof(char const*), NULL);

    built_from_ir(a, m);

    return a;
}

void asm_x86_64_drop(ASM_X86_64 *a) {
    for(size_t i=0; i<vector_len(a->insts); ++i) {
        ASM_X86_64_Inst* inst = vector_at(a->insts, i);
        asm_x86_64_inst_destruct(inst);
    }
    vector_drop(a->insts);

    vector_drop(a->values);
    vector_drop(a->global_values);
    vector_drop(a->offsets);
    uint_map_drop(a->labels);

    free(a);
}

static void fprint_inst_op(FILE* fp, char const* op, int num, ...);
static void fprint_value(FILE* fp, ASM_X86_64_Value* v);
static void fprint_reg(FILE* fp, ASM_X86_64_Reg reg);

static void asm_x86_64_set_val(ASM_X86_64 *a, IRSymbolID id, ASM_X86_64_Value value, int is_global) {
    Vector* mem = a->values;
    if (is_global) {
        mem = a->global_values;
    }

    while(vector_len(mem) <= id) {
        void* e = vector_append(mem);
        assert(e);
    }

    ASM_X86_64_Var* var = vector_at(mem, id);
    var->kind = ASM_X86_64_VAR_VAL;
    var->value.val = value;

    fprintf(DEBUGOUT, "SET = %ld <- ", id);
    fprint_value(DEBUGOUT, &var->value.val);
    fprintf(DEBUGOUT, "\n");
}

static ASM_X86_64_Value* asm_x86_64_get_val(ASM_X86_64 *a, IRSymbolID id, int is_global) {
    Vector* mem = a->values;
    if (is_global) {
        mem = a->global_values;
    }

    fprintf(DEBUGOUT, "GET = %ld\n", id);
    assert(id <= vector_len(mem));
    ASM_X86_64_Var* var = vector_at(mem, id);
    if (var->kind == ASM_X86_64_VAR_VAL) {
        return &var->value.val;
    }

    assert(0); // TODO: implement
}

static void asm_x86_64_set_local(ASM_X86_64 *a, IRSymbolID id, size_t offset) {
    while(vector_len(a->offsets) <= id) {
        void* e = vector_append(a->offsets);
        assert(e);
    }

    fprintf(DEBUGOUT, "SET LOCAL = %ld <- %ld\n", id, offset);
    size_t* o = vector_at(a->offsets, id);
    *o = offset;
}

static size_t asm_x86_64_get_local(ASM_X86_64 *a, IRSymbolID id) {
    fprintf(DEBUGOUT, "GET LOCAL = %ld\n", id);
    assert(id <= vector_len(a->offsets));

    size_t* o = vector_at(a->offsets, id);
    return *o;
}

void asm_x86_64_fprint(FILE* fp, ASM_X86_64* a) {
	fprintf(fp, ".file	\"simple_00.c\"\n"); // TODO
	fprintf(fp, ".text\n");                  // TODO

    for(size_t i=0; i<vector_len(a->insts); ++i) {
        ASM_X86_64_Inst* inst = vector_at(a->insts, i);
        switch(inst->kind) {
        case ASM_X86_64_INST_KIND_SECTION:
            fprintf(fp, "\t");
            fprintf(fp, ".section\t%s\n", inst->value.section.name);
            break;

        case ASM_X86_64_INST_KIND_STRING:
            fprintf(fp, "\t");
            fprintf(fp, ".string\t\"%s\"\n", inst->value.string.s);
            break;

        case ASM_X86_64_INST_KIND_TEXT:
            fprintf(fp, "\t");
            fprintf(fp, ".text\n");
            break;

        case ASM_X86_64_INST_KIND_GLOBAL:
            fprintf(fp, "\t");
            fprintf(fp, ".globl\t%s\n", inst->value.global.name);
            break;

        case ASM_X86_64_INST_KIND_TYPE:
            fprintf(fp, "\t");
            fprintf(fp, ".type\t%s, %s\n", inst->value.type.name, inst->value.type.type);
            break;

        case ASM_X86_64_INST_KIND_LABEL:
            fprintf(fp, "%s:\n", inst->value.label.name);
            break;

        case ASM_X86_64_INST_KIND_OP:
        {
            fprintf(fp, "\t");
            ASM_X86_64_Value* args = inst->value.op.args;
            switch(inst->value.op.op) {
            case ASM_X86_64_OP_PUSHQ:
                fprint_inst_op(fp, "pushq", 1, &args[0]);
                break;

            case ASM_X86_64_OP_POPQ:
                fprint_inst_op(fp, "popq", 1, &args[0]);
                break;

            case ASM_X86_64_OP_MOVQ:
                fprint_inst_op(fp, "movq", 2, &args[1], &args[0]);
                break;

            case ASM_X86_64_OP_ADDQ:
                fprint_inst_op(fp, "addq", 2, &args[1], &args[0]);
                break;

            case ASM_X86_64_OP_SUBQ:
                fprint_inst_op(fp, "subq", 2, &args[1], &args[0]);
                break;

            case ASM_X86_64_OP_LEAQ:
                fprint_inst_op(fp, "leaq", 2, &args[1], &args[0]);
                break;

            case ASM_X86_64_OP_CALL:
                fprint_inst_op(fp, "call", 1, &args[0]);
                break;

            case ASM_X86_64_OP_CMPQ:
                fprint_inst_op(fp, "cmp", 2, &args[1], &args[0]);
                break;

            case ASM_X86_64_OP_JMP:
                fprint_inst_op(fp, "jmp", 1, &args[0]);
                break;

            case ASM_X86_64_OP_JE:
                fprint_inst_op(fp, "je", 1, &args[0]);
                break;

            case ASM_X86_64_OP_RET:
                fprint_inst_op(fp, "ret", 0);
                break;

            default:
                assert(0); // TODO: error handling...
            }
            break;
        }
        }
    }
}

void fprint_inst_op(FILE* fp, char const* op, int num, ...) {
    va_list ap;

    fprintf(fp, "%s", op);

    va_start(ap, num);
    for(int i=0; i<num; ++i) {
        if (i == 0) {
            fprintf(fp, "\t");
        } else {
            fprintf(fp, ", ");
        }
        ASM_X86_64_Value* arg = va_arg(ap, ASM_X86_64_Value*);
        fprint_value(fp, arg);
    }
    va_end(ap);

    fprintf(fp, "\n");
}

void fprint_value(FILE* fp, ASM_X86_64_Value* v) {
    switch(v->kind) {
    case ASM_X86_64_VALUE_KIND_SYMBOL:
        fprintf(fp, "%s", v->value.symbol);
        break;

    case ASM_X86_64_VALUE_KIND_IMM_INT:
        fprintf(fp, "$%d", v->value.imm_int);
        break;

    case ASM_X86_64_VALUE_KIND_STRING:
        fprintf(fp, "%s(%%rip)", v->value.string.label);
        break;

    case ASM_X86_64_VALUE_KIND_REG:
        fprint_reg(fp, v->value.reg);
        break;

    case ASM_X86_64_VALUE_KIND_DISP_REG:
        if (v->value.disp_reg.symbol) {
            fprintf(fp, "%s", v->value.disp_reg.symbol);
        } else {
            fprintf(fp, "%d", v->value.disp_reg.disp);
        }
        fprintf(fp, "(");
        fprint_reg(fp, v->value.disp_reg.reg);
        fprintf(fp, ")");
        break;

    default:
        fprintf(stderr, "Unknown kind: %d", v->kind);
        assert(0); // TODO: error handling...
    }
}

void fprint_reg(FILE* fp, ASM_X86_64_Reg reg) {
    char const* reg_name;
    switch(reg) {
    case ASM_X86_64_REG_RAX:
        reg_name = "rax";
        break;

    case ASM_X86_64_REG_RBX:
        reg_name = "rbx";
        break;

    case ASM_X86_64_REG_RCX:
        reg_name = "rcx";
        break;

    case ASM_X86_64_REG_RDX:
        reg_name = "rdx";
        break;

    case ASM_X86_64_REG_RSP:
        reg_name = "rsp";
        break;

    case ASM_X86_64_REG_RBP:
        reg_name = "rbp";
        break;

    case ASM_X86_64_REG_RSI:
        reg_name = "rsi";
        break;

    case ASM_X86_64_REG_RDI:
        reg_name = "rdi";
        break;

    case ASM_X86_64_REG_RIP:
        reg_name = "rip";
        break;

    case ASM_X86_64_REG_EAX:
        reg_name = "eax";
        break;

    default:
        fprintf(stderr, "Unexpected kind: %d\n", reg);
        assert(0); // TODO: error handling...
    }

    fprintf(fp, "%%%s", reg_name);
}

void built_from_ir(ASM_X86_64 *a, IRModule* m) {
    for(size_t i=0; i<vector_len(m->definitions); ++i) {
        IRInst* inst = vector_at(m->definitions, i);
        built_from_ir_global_inst(a, inst);
    }

    for(size_t i=0; i<vector_len(m->functions); ++i) {
        IRFunction* f = vector_at(m->functions, i);
        built_from_ir_function(a, f);
    }
}

static void built_from_ir_function_bb_collect_labels_iter(IRBB* bb, void* args) {
    ASM_X86_64* a = (ASM_X86_64*)args;
    collect_labels_from_ir_bb(a, bb);
}

static void built_from_ir_function_bb_built_iter(IRBB* bb, void* args) {
    ASM_X86_64* a = (ASM_X86_64*)args;
    built_from_ir_bb(a, bb);
}

void built_from_ir_function(ASM_X86_64 *a, IRFunction* f) {
    {
        // .text
        ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
        inst->kind = ASM_X86_64_INST_KIND_TEXT;
    }

    // TODO: fix
    {
        // .global
        ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
        inst->kind = ASM_X86_64_INST_KIND_GLOBAL;
        inst->value.global.name = f->name;
    }

    {
        // .type
        ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
        inst->kind = ASM_X86_64_INST_KIND_TYPE;
        inst->value.type.name = f->name;
        inst->value.type.type = "@function";
    }

    {
        // label
        ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
        inst->kind = ASM_X86_64_INST_KIND_LABEL;
        inst->value.label.name = f->name;
        inst->value.label.generated = 0;
    }

    {
        // pushq rbp
        ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
        inst->kind = ASM_X86_64_INST_KIND_OP;
        inst->value.op.op = ASM_X86_64_OP_PUSHQ;
        inst->value.op.args[0].kind = ASM_X86_64_VALUE_KIND_REG;
        inst->value.op.args[0].value.reg = ASM_X86_64_REG_RBP;
    }

    {
        // movq rbp, rsp
        ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
        inst->kind = ASM_X86_64_INST_KIND_OP;
        inst->value.op.op = ASM_X86_64_OP_MOVQ;
        inst->value.op.args[0].kind = ASM_X86_64_VALUE_KIND_REG;
        inst->value.op.args[0].value.reg = ASM_X86_64_REG_RBP;
        inst->value.op.args[1].kind = ASM_X86_64_VALUE_KIND_REG;
        inst->value.op.args[1].value.reg = ASM_X86_64_REG_RSP;
    }

    size_t stack_size = 0;
    for(size_t i=0; i<vector_len(f->locals); ++i) {
        size_t* local = vector_at(f->locals, i);

        asm_x86_64_set_local(a, i, stack_size);
        assert(*local <= 16);
        stack_size += *local;
    }

    if (stack_size != 0) {
        // subq rsp, 'stack_size'
        ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
        inst->kind = ASM_X86_64_INST_KIND_OP;
        inst->value.op.op = ASM_X86_64_OP_SUBQ;
        inst->value.op.args[0].kind = ASM_X86_64_VALUE_KIND_REG;
        inst->value.op.args[0].value.reg = ASM_X86_64_REG_RSP;
        inst->value.op.args[1].kind = ASM_X86_64_VALUE_KIND_IMM_INT;
        inst->value.op.args[1].value.imm_int = stack_size;
    }

    ir_bb_visit(f->entry, built_from_ir_function_bb_collect_labels_iter, a);
    ir_bb_visit(f->entry, built_from_ir_function_bb_built_iter, a);
}

static char const* make_code_label(size_t index) {
    char* label_id = (char*)malloc(sizeof(char) * 10);
    snprintf(label_id, 10, ".L%07ld", index);

    return label_id;
}

void collect_labels_from_ir_bb(ASM_X86_64 *a, IRBB* bb) {
    char const* label_id = make_code_label(a->code_label_count);
    char const** m = uint_map_insert(a->labels, bb->id, NULL);
    *m = label_id;

    a->code_label_count++;
}

void built_from_ir_bb(ASM_X86_64 *a, IRBB* bb) {
    char const** m = uint_map_find(a->labels, bb->id);
    assert(m);
    char const* label_id = *m;

    {
        // .label 'label_id'
        ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
        inst->kind = ASM_X86_64_INST_KIND_LABEL;
        inst->value.label.name = label_id;
        inst->value.label.generated = 1;
    }

    for(size_t i=0; i<vector_len(bb->insts); ++i) {
        IRInst* inst = vector_at(bb->insts, i);
        built_from_ir_inst(a, inst);
    }
    assert(bb->term);
    built_from_ir_inst(a, bb->term);
}

void built_from_ir_inst(ASM_X86_64 *a, IRInst* inst) {
    switch(inst->kind) {
    case IR_INST_KIND_LET:
    {
        IRSymbolID var_id = inst->value.let.id;
        IRInstValue* let_rhs = &inst->value.let.rhs;
        switch(let_rhs->kind) {
        case IR_INST_VALUE_KIND_REF:
        {
            ASM_X86_64_Value* ref_val =
                asm_x86_64_get_val(a, let_rhs->value.ref.sym, let_rhs->value.ref.is_global);

            asm_x86_64_set_val(a, var_id, *ref_val, 0);

            break;
        }

        case IR_INST_VALUE_KIND_ADDR_OF:
        {
            ASM_X86_64_Value* ref_val = asm_x86_64_get_val(a, let_rhs->value.addr_of.sym, 0);

            ASM_X86_64_Value dest = {
                .kind = ASM_X86_64_VALUE_KIND_REG,
                .value = {
                    .reg = ASM_X86_64_REG_RAX,
                },
            };

            // leaq RAX, 'ref_val'
            ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
            inst->kind = ASM_X86_64_INST_KIND_OP;
            inst->value.op.op = ASM_X86_64_OP_LEAQ;
            inst->value.op.args[0] = dest;
            inst->value.op.args[1] = *ref_val;

            asm_x86_64_set_val(a, var_id, dest, 0);

            break;
        }

        case IR_INST_VALUE_KIND_IMM_INT:
        {
            ASM_X86_64_Value value = {
                .kind = ASM_X86_64_VALUE_KIND_IMM_INT,
                .value = {
                    .imm_int = let_rhs->value.imm_int, // TODO: fix size
                },
            };
            asm_x86_64_set_val(a, var_id, value, 0);

            break;
        }

        case IR_INST_VALUE_KIND_OP_BIN:
        {
            // TODO: !!! implement correctly !!!

            ASM_X86_64_Value* lhs_val = asm_x86_64_get_val(a, let_rhs->value.op_bin.lhs, 0);
            ASM_X86_64_Value* rhs_val = asm_x86_64_get_val(a, let_rhs->value.op_bin.rhs, 0);

            // Move value into RAX (TODO: fix size of data)
            {
                // movq rax, 'lhs_val'
                ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
                inst->kind = ASM_X86_64_INST_KIND_OP;
                inst->value.op.op = ASM_X86_64_OP_MOVQ;
                inst->value.op.args[0].kind = ASM_X86_64_VALUE_KIND_REG;
                inst->value.op.args[0].value.reg = ASM_X86_64_REG_RAX;
                inst->value.op.args[1] = *lhs_val;
            }

            // Add value to RAX (TODO: fix size of data)
            {
                // addq rax, 'rhs_val'
                ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
                inst->kind = ASM_X86_64_INST_KIND_OP;
                inst->value.op.op = ASM_X86_64_OP_ADDQ;
                inst->value.op.args[0].kind = ASM_X86_64_VALUE_KIND_REG;
                inst->value.op.args[0].value.reg = ASM_X86_64_REG_RAX;
                inst->value.op.args[1] = *rhs_val;
            }

            // Result is saved in RAX (TODO: fix size of data)
            size_t var_target_offset = asm_x86_64_get_local(a, var_id);
            ASM_X86_64_Value dest = {
                .kind = ASM_X86_64_VALUE_KIND_DISP_REG,
                .value = {
                    .disp_reg = {
                        .symbol = NULL,
                        .disp = (int)var_target_offset, // TODO: fix
                        .reg = ASM_X86_64_REG_RSP,
                    },
                },
            };
            {
                // movq 'var_target(RSP)', RAX
                ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
                inst->kind = ASM_X86_64_INST_KIND_OP;
                inst->value.op.op = ASM_X86_64_OP_ADDQ;
                inst->value.op.args[0] = dest;
                inst->value.op.args[1].kind = ASM_X86_64_VALUE_KIND_REG;
                inst->value.op.args[1].value.reg = ASM_X86_64_REG_RAX;
            }

            asm_x86_64_set_val(a, var_id, dest, 0);

            break;
        }

        case IR_INST_VALUE_KIND_CALL:
        {
            IRSymbolID lhs_id = let_rhs->value.call.lhs;
            ASM_X86_64_Value* lhs_val = asm_x86_64_get_val(a, lhs_id, 0);

            fprintf(DEBUGOUT, "CALL LHS %ld = ", lhs_id);
            fprint_value(DEBUGOUT, lhs_val);
            fprintf(DEBUGOUT, "\n");

            // TODO: support register passing
            for(size_t i=0; i<vector_len(let_rhs->value.call.args); ++i) {
                assert(i<sizeof(arg_regs)/sizeof(ASM_X86_64_Reg));

                IRSymbolID* arg = vector_at(let_rhs->value.call.args, i);
                ASM_X86_64_Value* arg_val = asm_x86_64_get_val(a, *arg, 0);

                // movq ARG_REG, 'arg_val'
                ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
                inst->kind = ASM_X86_64_INST_KIND_OP;
                inst->value.op.op = ASM_X86_64_OP_MOVQ;
                inst->value.op.args[0].kind = ASM_X86_64_VALUE_KIND_REG;
                inst->value.op.args[0].value.reg = arg_regs[i];
                inst->value.op.args[1] = *arg_val;
            }

            {
                // call 'lhs_val'
                ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
                inst->kind = ASM_X86_64_INST_KIND_OP;
                inst->value.op.op = ASM_X86_64_OP_CALL;
                inst->value.op.args[0] = *lhs_val;
            }

            // Result is saved in RAX (TODO: fix size of data)
            size_t var_target_offset = asm_x86_64_get_local(a, var_id);
            ASM_X86_64_Value dest = {
                .kind = ASM_X86_64_VALUE_KIND_DISP_REG,
                .value = {
                    .disp_reg = {
                        .symbol = NULL,
                        .disp = (int)var_target_offset, // TODO: fix
                        .reg = ASM_X86_64_REG_RSP,
                    },
                },
            };
            {
                // movq 'var_target(RSP)', RAX
                ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
                inst->kind = ASM_X86_64_INST_KIND_OP;
                inst->value.op.op = ASM_X86_64_OP_MOVQ;
                inst->value.op.args[0] = dest;
                inst->value.op.args[1].kind = ASM_X86_64_VALUE_KIND_REG;
                inst->value.op.args[1].value.reg = ASM_X86_64_REG_RAX;
            }

            asm_x86_64_set_val(a, var_id, dest, 0);

            break;
        }

        default:
            fprintf(stderr, "Unexpected kind: %d\n", let_rhs->kind);
            assert(0); // TODO: error handling
        }
        break;
    }

    case IR_INST_KIND_RET:
    {
        IRSymbolID var_id = inst->value.ret.id;
        ASM_X86_64_Value* ret_val = asm_x86_64_get_val(a, var_id, 0);

        // Set a result (TODO: fix size of data)
        {
            // movq rax, 'ret_val'
            ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
            inst->kind = ASM_X86_64_INST_KIND_OP;
            inst->value.op.op = ASM_X86_64_OP_MOVQ;
            inst->value.op.args[0].kind = ASM_X86_64_VALUE_KIND_REG;
            inst->value.op.args[0].value.reg = ASM_X86_64_REG_RAX;
            inst->value.op.args[1] = *ret_val;
        }

        {
            // movq rsp, rbp
            ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
            inst->kind = ASM_X86_64_INST_KIND_OP;
            inst->value.op.op = ASM_X86_64_OP_MOVQ;
            inst->value.op.args[0].kind = ASM_X86_64_VALUE_KIND_REG;
            inst->value.op.args[0].value.reg = ASM_X86_64_REG_RSP;
            inst->value.op.args[1].kind = ASM_X86_64_VALUE_KIND_REG;
            inst->value.op.args[1].value.reg = ASM_X86_64_REG_RBP;
        }

        {
            // popq rbp
            ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
            inst->kind = ASM_X86_64_INST_KIND_OP;
            inst->value.op.op = ASM_X86_64_OP_POPQ;
            inst->value.op.args[0].kind = ASM_X86_64_VALUE_KIND_REG;
            inst->value.op.args[0].value.reg = ASM_X86_64_REG_RBP;
        }

        {
            // ret
            ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
            inst->kind = ASM_X86_64_INST_KIND_OP;
            inst->value.op.op = ASM_X86_64_OP_RET;
        }

        break;
    }

    case IR_INST_KIND_BRANCH:
    {
        ASM_X86_64_Value* cond_val = asm_x86_64_get_val(a, inst->value.branch.cond, 0);

        {
            // movq RAX, 'cond'
            ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
            inst->kind = ASM_X86_64_INST_KIND_OP;
            inst->value.op.op = ASM_X86_64_OP_MOVQ;

            inst->value.op.args[0].kind = ASM_X86_64_VALUE_KIND_REG;
            inst->value.op.args[0].value.reg = ASM_X86_64_REG_RAX;
            inst->value.op.args[1] = *cond_val;
        }

        {
            // cmpq RAX, 0
            ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
            inst->kind = ASM_X86_64_INST_KIND_OP;
            inst->value.op.op = ASM_X86_64_OP_CMPQ;
            inst->value.op.args[0].kind = ASM_X86_64_VALUE_KIND_REG;
            inst->value.op.args[0].value.reg = ASM_X86_64_REG_RAX;
            inst->value.op.args[1].kind = ASM_X86_64_VALUE_KIND_IMM_INT;
            inst->value.op.args[1].value.imm_int = 0;
        }

        {
            char const** m = uint_map_find(a->labels, inst->value.branch.else_bb->id);
            assert(m);
            char const* label_id = *m;

            // JE else_bb
            ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
            inst->kind = ASM_X86_64_INST_KIND_OP;
            inst->value.op.op = ASM_X86_64_OP_JE;
            inst->value.op.args[0].kind = ASM_X86_64_VALUE_KIND_SYMBOL;
            inst->value.op.args[0].value.symbol = label_id;
        }

        {
            char const** m = uint_map_find(a->labels, inst->value.branch.then_bb->id);
            assert(m);
            char const* label_id = *m;

            // JMP then_bb
            ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
            inst->kind = ASM_X86_64_INST_KIND_OP;
            inst->value.op.op = ASM_X86_64_OP_JMP;
            inst->value.op.args[0].kind = ASM_X86_64_VALUE_KIND_SYMBOL;
            inst->value.op.args[0].value.symbol = label_id;
        }

        break;
    }

    case IR_INST_KIND_JUMP:
    {
        char const** m = uint_map_find(a->labels, inst->value.jump.next_bb->id);
        assert(m);
        char const* label_id = *m;

        ASM_X86_64_Value label = {
            .kind = ASM_X86_64_VALUE_KIND_SYMBOL,
            .value = {
                .symbol = label_id,
            },
        };

        // jmp 'label'
        ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
        inst->kind = ASM_X86_64_INST_KIND_OP;
        inst->value.op.op = ASM_X86_64_OP_JMP;
        inst->value.op.args[0] = label;

        break;
    }

    default:
        assert(0); // TODO: error handling
    }
}

void built_from_ir_global_inst(ASM_X86_64 *a, IRInst* inst) {
    assert(inst->kind == IR_INST_KIND_LET);
    IRSymbolID var_id = inst->value.let.id;
    IRInstValue* let_rhs = &inst->value.let.rhs;

    switch(let_rhs->kind) {
    case IR_INST_VALUE_KIND_SYMBOL:
    {
        // Save a symbol
        ASM_X86_64_Value value = {
            .kind = ASM_X86_64_VALUE_KIND_SYMBOL,
            .value = {
                .symbol = let_rhs->value.symbol,
            },
        };
        asm_x86_64_set_val(a, var_id, value, 1);

        fprintf(DEBUGOUT, "SYMBOL %ld = ", var_id);
        fprint_value(DEBUGOUT, &value);
        fprintf(DEBUGOUT, "\n");

        break;
    }

    case IR_INST_VALUE_KIND_STRING:
    {
        char* label_id = (char*)malloc(sizeof(char) * 10);
        snprintf(label_id, 10, ".S%07ld", a->string_label_count);
        a->string_label_count++;

        {
            // .section .rodata
            ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
            inst->kind = ASM_X86_64_INST_KIND_SECTION;
            inst->value.section.name = ".rodata";
        }

        {
            // .label 'label_id'
            ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
            inst->kind = ASM_X86_64_INST_KIND_LABEL;
            inst->value.label.name = label_id;
            inst->value.label.generated = 1;
        }

        {
            // .string 'string'
            ASM_X86_64_Inst* inst = (ASM_X86_64_Inst*)vector_append(a->insts);
            inst->kind = ASM_X86_64_INST_KIND_STRING;
            inst->value.string.s = let_rhs->value.string;
        }

        ASM_X86_64_Value str = {
            .kind = ASM_X86_64_VALUE_KIND_STRING,
            .value = {
                .string = {
                    .label = label_id,
                },
            },
        };

        asm_x86_64_set_val(a, var_id, str, 1);

        fprintf(DEBUGOUT, "STRING %ld = ", var_id);
        fprint_value(DEBUGOUT, &str);
        fprintf(DEBUGOUT, "\n");

        break;
    }

    default:
        fprintf(stderr, "Unexpected kind: %d\n", let_rhs->kind);
        assert(0); // TODO: error handling
    }
}
