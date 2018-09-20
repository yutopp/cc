#include "ir_bb_arena.h"
#include "ir_bb.h"

IRBBArena* ir_bb_arena_new() {
    return (IRBBArena*)arena_new(sizeof(IRBB), (void (*)(void *))ir_bb_destruct);
}

void ir_bb_arena_drop(IRBBArena* arena) {
    arena_drop(arena);
}

IRBB* ir_bb_arena_malloc(IRBBArena *arena) {
    return (IRBB*)arena_malloc(arena);
}
