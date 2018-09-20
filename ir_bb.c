#include <stdlib.h>
#include <assert.h>
#include "ir_bb.h"
#include "ir_inst_defs.h"
#include "map.h"

void ir_bb_construct(IRBB* bb, IRBBID id) {
    bb->id = id;
    bb->prevs = vector_new(sizeof(IRBB*));
    bb->insts = vector_new(sizeof(IRInst));
    bb->term = NULL;
}

void ir_bb_destruct(IRBB* bb) {
    vector_drop(bb->prevs);

    for(size_t i=0; i<vector_len(bb->insts); ++i) {
        IRInst* inst = vector_at(bb->insts, i);
        ir_inst_destruct(inst);
    }
    vector_drop(bb->insts);

    if (bb->term) {
        ir_inst_destruct(bb->term);
        free(bb->term);
    }
}

void ir_bb_foreach_nexts(IRBB* bb, void(*f)(IRBB*, void*), void* args) {
    if (bb->term == NULL) {
        return;
    }

    switch(bb->term->kind) {
    case IR_INST_KIND_RET:
        return;

    case IR_INST_KIND_BRANCH:
    {
        f( bb->term->value.branch.then_bb, args);
        if (bb->term->value.branch.else_bb) {
            f(bb->term->value.branch.else_bb, args);
        }
        return;
    }

    case IR_INST_KIND_JUMP:
    {
        f(bb->term->value.jump.next_bb, args);
        return;
    }

    default:
        assert(0); // TODO: error handling
        return;
    }
}

struct ir_bb_visit_iter_arg_t {
    UintMap* visited;
    Vector* bbs; // Vector<IRBB*>
    size_t head;
};

static void ir_bb_visit_iter(IRBB* next, void* args) {
    struct ir_bb_visit_iter_arg_t* state = (struct ir_bb_visit_iter_arg_t*)args;

    int exist;
    uint_map_insert(state->visited, next->id, &exist);
    if (exist == 0) {
        IRBB** t = vector_append(state->bbs);
        *t = next;
    }
}

void ir_bb_visit(IRBB* initial_bb, void(*f)(IRBB* bb, void*), void* args) {
    struct ir_bb_visit_iter_arg_t state = {
        .visited = uint_map_new(1, NULL),
        .bbs = vector_new(sizeof(IRBB*)), // Vector<IRBB*>
        .head = 0,
    };

    IRBB** bb = vector_append(state.bbs);
    *bb = initial_bb;

    while(state.head < vector_len(state.bbs)) {
        bb = vector_at(state.bbs, state.head);
        state.head++;

        f(*bb, args);

        ir_bb_foreach_nexts(*bb, ir_bb_visit_iter, &state);
    }

    vector_drop(state.bbs);
    uint_map_drop(state.visited);
}
