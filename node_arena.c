#include "node_arena.h"
#include "arena.h"

NodeArena* node_arena_new() {
    return (NodeArena*)arena_new(sizeof(Node), (void (*)(void *))node_destruct);
}

Node* node_arena_malloc(NodeArena *arena) {
    return (Node*)arena_malloc(arena);
}

void node_arena_drop(NodeArena *arena) {
    arena_drop(arena);
}
