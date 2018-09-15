#ifndef CC_ARENA_H
#define CC_ARENA_H

#include "node.h"

struct arena_t;
typedef struct arena_t Arena;

Arena* arena_new();
Node* arena_malloc(Arena *arena);
void arena_drop(Arena *arena);

#endif /* CC_ARENA_H */
