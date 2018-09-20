#ifndef CC_IR_H
#define CC_IR_H

#include <stdio.h>
#include "ir_inst.h"
#include "ir_bb.h"
#include "ir_bb_arena.h"
#include "node.h"

typedef size_t IRSymbolID;

struct ir_function_t;
typedef struct ir_function_t IRFunction;

void ir_function_set_local(IRFunction* f, IRSymbolID id, size_t size);
size_t ir_function_get_local(IRFunction* f, IRSymbolID id);

struct ir_module_t;
typedef struct ir_module_t IRModule;

// TODO: encapsulate
struct ir_function_t {
    char const* name;
    IRModule* mod;          // reference
    IRBBArena* bb_arena;
    IRBB* entry;            // reference
    IRBBID bb_id;
    Vector* locals;         // Vector<size_t/*type_t*/> TODO: Change to type info
    IRSymbolID locals_id;
};

// TODO: encapsulate
struct ir_module_t {
    Vector* definitions;    // Vector<IRInst>
    Vector* functions;      // Vector<IRFunction>
    IRSymbolID defs_sym_id;
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
