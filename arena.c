#include <stdlib.h>
#include "arena.h"
#include "vector.h"
#include "node.h"

struct arena_t {
    Vector* current; // Vector<Node>
    Vector* chain;   // Vector<Vector<Node>>
};

Arena* arena_new() {
    Arena* v = (Arena*)malloc(sizeof(Arena));
    if (!v) {
        return 0;
    }
    v->current = vector_new_with_cap(sizeof(Node), 128);
    v->chain = vector_new(sizeof(Vector*));

    return v;
}

Node* arena_malloc(Arena *arena) {
    if (vector_len(arena->current) == vector_cap(arena->current)) {
        // Move an ownership of current into chain
        Vector** c = (Vector**)vector_append(arena->chain);
        (*c) = arena->current;

        // Allocate a new space
        arena->current = vector_new_with_cap(sizeof(Node), vector_cap(arena->current) * 2);

        return arena_malloc(arena);
    }

    return (Node*)vector_append(arena->current);
}

void arena_drop(Arena *arena) {
    if (!arena) {
        return;
    }

    for(int i=0; i<vector_len(arena->current); ++i) {
        Node* n = (Node*)vector_at(arena->current, i);
        node_drop(n);
    }
    vector_drop(arena->current);

    for(int i=0; i<vector_len(arena->chain); ++i) {
        Vector** c = (Vector**)vector_at(arena->chain, i);
        for(int j=0; j<vector_len(*c); ++j) {
            Node* n = (Node*)vector_at(*c, j);
            node_drop(n);
        }
        vector_drop(*c);
    }
    vector_drop(arena->chain);

    free(arena);
}
