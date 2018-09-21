#ifndef CC_NODE_ARENA_H
#define CC_NODE_ARENA_H

#include "node.h"

typedef struct arena_t NodeArena;

NodeArena* node_arena_new();
void node_arena_drop(NodeArena* arena);

Node* node_arena_malloc(NodeArena *arena);

#endif /* CC_NODE_ARENA_H */
