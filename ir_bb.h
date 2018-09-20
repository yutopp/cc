#ifndef CC_IR_BB_H
#define CC_IR_BB_H

#include "ir_inst.h"
#include "vector.h"

typedef size_t IRBBID;

struct ir_bb_t;
typedef struct ir_bb_t IRBB;

// TODO: encapsulate
struct ir_bb_t {
    IRBBID id;
    Vector* prevs; // Vector<IRBB*>
    Vector* insts; // Vector<IRInst>
    IRInst* term;
};

void ir_bb_construct(IRBB*, IRBBID id);
void ir_bb_destruct(IRBB*);

void ir_bb_foreach_nexts(IRBB* bb, void(*f)(IRBB* next, void*), void* args);
void ir_bb_visit(IRBB* initial_bb, void(*f)(IRBB* bb, void*), void* args);

#endif /* CC_IR_BB_H */
