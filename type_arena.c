#include "type_arena.h"
#include "arena.h"

TypeArena* type_arena_new() {
    return (TypeArena*)arena_new(sizeof(Type), (void (*)(void *))type_destruct);
}

void type_arena_drop(TypeArena* arena) {
    arena_drop(arena);
}

Type* type_arena_malloc(TypeArena *arena) {
    return (Type*)arena_malloc(arena);
}
