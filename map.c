#include <stdlib.h>
#include "map.h"
#include "vector.h"

typedef struct elem_t {
    union {
        size_t num;
        char const* name;
    } key;
    char* buffer; // TODO: consider alignment
} Elem;

struct map_t {
    size_t elem_size;
    Vector* elems; // Vector<Elem>
    void (*dtor)(void*);
};

UintMap* uint_map_new(size_t elem_size, void (*dtor)(void*)) {
    UintMap* m = (UintMap*)malloc(sizeof(UintMap));
    m->elem_size = elem_size;
    m->elems = vector_new(sizeof(Elem));
    m->dtor = dtor;

    return m;
}

void uint_map_drop(UintMap* m) {
    for(size_t i=0; i<vector_len(m->elems); ++i) {
        Elem* elem = vector_at(m->elems, i);
        if (m->dtor) {
            m->dtor((void*)elem->buffer);
        }
        free(elem->buffer);

    }
    vector_drop(m->elems);

    free(m);
}

void* uint_map_insert(UintMap* m, size_t key, int* exist) {
    // TODO: improve performance
    void* content = uint_map_find(m, key);
    if (content) {
        if (exist) { *exist = 1; };
        return content;
    }

    if (exist) { *exist = 0; };
    Elem* elem = (Elem*)vector_append(m->elems);
    elem->key.num = key;
    elem->buffer = (char*)malloc(m->elem_size); // TODO: consider alignment

    return (void*)elem->buffer;
}

void* uint_map_find(UintMap* m, size_t key) {
    // TODO: improve performance
    for(size_t i=0; i<vector_len(m->elems); ++i) {
        Elem* elem = vector_at(m->elems, i);
        if (elem->key.num == key) {
            return (void*)elem->buffer;
        }
    }
    return NULL;
}
