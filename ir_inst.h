#ifndef CC_IR_INST_H
#define CC_IR_INST_H

#include "vector.h"

typedef size_t IRSymbolID; // TODO: fix

typedef enum ir_inst_value_kind_t IRInstValueKind;

struct ir_inst_value_t;
typedef struct ir_inst_value_t IRInstValue;

void ir_inst_value_destruct(IRInstValue* v);

typedef enum ir_inst_kind_t IRInstKind;

struct ir_inst_t;
typedef struct ir_inst_t IRInst;

void ir_inst_destruct(IRInst* inst);

#endif /* CC_IR_INST_H */
