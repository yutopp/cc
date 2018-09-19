#include <stdlib.h>
#include "arena.h"
#include "vector.h"

struct arena_t {
    Vector* current; // Vector<T>
    Vector* chain;   // Vector<Vector<T>>
    size_t elem_size;
    void (*dtor)(void*);
};

Arena* arena_new(size_t elem_size, void (*dtor)(void*)) {
    Arena* v = (Arena*)malloc(sizeof(Arena));
    if (!v) {
        return 0;
    }
    v->current = vector_new_with_cap(elem_size, 128);
    v->chain = vector_new(sizeof(Vector*));
    v->elem_size = elem_size;
    v->dtor = dtor;

    return v;
}

void* arena_malloc(Arena *arena) {
    if (vector_len(arena->current) == vector_cap(arena->current)) {
        // Move an ownership of current into chain
        Vector** c = (Vector**)vector_append(arena->chain);
        (*c) = arena->current;

        // Allocate a new space
        arena->current = vector_new_with_cap(arena->elem_size, vector_cap(arena->current) * 2);

        return arena_malloc(arena);
    }

    return vector_append(arena->current);
}

void arena_drop(Arena *arena) {
    if (!arena) {
        return;
    }

    for(size_t i=0; i<vector_len(arena->current); ++i) {
        void* e = vector_at(arena->current, i);
        arena->dtor(e);
    }
    vector_drop(arena->current);

    for(size_t i=0; i<vector_len(arena->chain); ++i) {
        Vector** c = (Vector**)vector_at(arena->chain, i);
        for(size_t j=0; j<vector_len(*c); ++j) {
            void* e = vector_at(*c, j);
            arena->dtor(e);
        }
        vector_drop(*c);
    }
    vector_drop(arena->chain);

    free(arena);
}
