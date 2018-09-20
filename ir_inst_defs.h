#ifndef CC_IR_INST_DEFS_H
#define CC_IR_INST_DEFS_H

#include "ir_inst.h"
#include "ir_bb.h"
#include "token.h"

enum ir_inst_value_kind_t {
    IR_INST_VALUE_KIND_SYMBOL,
    IR_INST_VALUE_KIND_STRING,
    IR_INST_VALUE_KIND_REF,
    IR_INST_VALUE_KIND_ADDR_OF,
    IR_INST_VALUE_KIND_IMM_INT,
    IR_INST_VALUE_KIND_OP_BIN,
    IR_INST_VALUE_KIND_CALL,
};

// TODO: encapsulate
struct ir_inst_value_t {
    IRInstValueKind kind;
    union {
        char const* symbol;
        char const* string;
        struct {
            int is_global;
            IRSymbolID sym;
        } ref;
        struct {
            IRSymbolID sym;
        } addr_of;
        int imm_int;
        struct {
            Token* op; // TODO: fix
            IRSymbolID lhs;
            IRSymbolID rhs;
        } op_bin;
        struct {
            IRSymbolID lhs;
            Vector* args; // Vector<IRSymbolID>
        } call;
    } value;
};

enum ir_inst_kind_t {
    IR_INST_KIND_LET,
    IR_INST_KIND_RET,
    IR_INST_KIND_BRANCH,
    IR_INST_KIND_JUMP,
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
        struct {
            IRSymbolID cond;
            IRBB* then_bb;
            IRBB* else_bb;
        } branch;
        struct {
            IRBB* next_bb;
        } jump;
    } value;
};

#endif /* CC_IR_INST_DEFS_H */
