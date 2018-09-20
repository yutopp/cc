#include <stdlib.h>
#include "ir_inst.h"
#include "ir_inst_defs.h"

void ir_inst_value_destruct(IRInstValue* v) {
    switch(v->kind) {
    case IR_INST_VALUE_KIND_SYMBOL:
        free((char*)v->value.symbol);
        break;

    case IR_INST_VALUE_KIND_STRING:
    case IR_INST_VALUE_KIND_REF:
    case IR_INST_VALUE_KIND_ADDR_OF:
    case IR_INST_VALUE_KIND_IMM_INT:
    case IR_INST_VALUE_KIND_OP_BIN:
        break; // DO NOTHING

    case IR_INST_VALUE_KIND_CALL:
        vector_drop(v->value.call.args);
        break;
    }
}

void ir_inst_destruct(IRInst* inst) {
    if (inst->kind == IR_INST_KIND_LET) {
        ir_inst_value_destruct(&inst->value.let.rhs);
    }
}
