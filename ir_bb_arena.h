#ifndef CC_IR_BB_ARENA_H
#define CC_IR_BB_ARENA_H

#include "arena.h"
#include "ir_bb.h"

typedef struct arena_t IRBBArena;

IRBBArena* ir_bb_arena_new();
void ir_bb_arena_drop(IRBBArena* arena);

IRBB* ir_bb_arena_malloc(IRBBArena *arena);

#endif /* CC_IR_BB_ARENA_H */
