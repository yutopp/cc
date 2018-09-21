#ifndef CC_TYPE_ARENA_H
#define CC_TYPE_ARENA_H

#include "type.h"

typedef struct arena_t TypeArena;

TypeArena* type_arena_new();
void type_arena_drop(TypeArena* arena);

Type* type_arena_malloc(TypeArena *arena);

#endif /* CC_TYPE_ARENA_H */
