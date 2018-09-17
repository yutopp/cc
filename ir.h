#ifndef CC_IR_H
#define CC_IR_H

#include <stdio.h>
#include "node.h"

typedef size_t IRSymbolID;

typedef enum ir_inst_value_kind_t {
    IR_INST_VALUE_KIND_SYMBOL,
    IR_INST_VALUE_KIND_IMM_INT,
    IR_INST_VALUE_KIND_OP_BIN,
    IR_INST_VALUE_KIND_CALL,
} IRInstValueKind;

struct ir_inst_value_t;
typedef struct ir_inst_value_t IRInstValue;

typedef enum ir_inst_kind_t {
    IR_INST_KIND_LET,
    IR_INST_KIND_RET,
} IRInstKind;

struct ir_inst_t;
typedef struct ir_inst_t IRInst;

struct ir_bb_t;
typedef struct ir_bb_t IRBB;

struct ir_function_t;
typedef struct ir_function_t IRFunction;

struct ir_module_t;
typedef struct ir_module_t IRModule;

// TODO: encapsulate
struct ir_inst_value_t {
    IRInstValueKind kind;
    union {
        char const* symbol;
        int imm_int;
        struct {
            Token* op;
            IRSymbolID lhs;
            IRSymbolID rhs;
        } op_bin;
        struct {
            IRSymbolID lhs;
            Vector* args; // Vector<IRSymbolID>
        } call;
    } value;
};

// TODO: encapsulate
struct ir_inst_t {
    IRInstKind kind;
    union {
        struct {
            IRSymbolID id;
            IRInstValue rhs;
        } let;
        struct {
            IRSymbolID id;
        } ret;
    } value;
};

// TODO: encapsulate
struct ir_bb_t {
    IRBB* prev;
    IRBB* next;
    Vector* insts; // Vector<IRInst>
};

// TODO: encapsulate
struct ir_function_t {
    char const* name;
    IRModule* mod;
    IRBB* entry;
};

// TODO: encapsulate
struct ir_module_t {
    Vector* definitions;    // Vector<IRInst>
    Vector* functions;      // Vector<IRFunction>
    IRSymbolID value_sym_id;
};

IRModule* ir_module_new();
void ir_module_drop(IRModule* m);

void ir_module_fprint(FILE* fp, IRModule* m);
void ir_function_fprint(FILE* fp, IRFunction* f);
void ir_bb_fprint(FILE* fp, IRBB* bb);
void ir_inst_fprint(FILE* fp, IRInst* inst);

struct ir_builder_t;
typedef struct ir_builder_t IRBuilder;

IRBuilder* ir_builder_new();
void ir_builder_drop(IRBuilder* builder);

IRModule* ir_builder_new_module(IRBuilder* builder, Node* node);

#endif /*CC_IR_H*/
