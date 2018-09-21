#include <stdlib.h>
#include <string.h>
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
typedef struct map_t Map;

static Map* map_new(size_t elem_size, void (*dtor)(void*)) {
    Map* m = (Map*)malloc(sizeof(Map));
    m->elem_size = elem_size;
    m->elems = vector_new(sizeof(Elem));
    m->dtor = dtor;

    return m;
}

static void map_drop(UintMap* m) {
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

static int uint_cmp(Elem* elem, void* arg) {
    return elem->key.num == *(size_t*)arg;
}

static int string_cmp(Elem* elem, void* arg) {
    return strcmp(elem->key.name, *(char const**)arg) == 0;
}

static void* map_find(UintMap* m, int (*cmp)(Elem*, void*), void* arg) {
    // TODO: improve performance
    for(size_t i=0; i<vector_len(m->elems); ++i) {
        Elem* elem = vector_at(m->elems, i);
        if (cmp(elem, arg)) {
            return (void*)elem->buffer;
        }
    }
    return NULL;
}

//
UintMap* uint_map_new(size_t elem_size, void (*dtor)(void*)) {
    return map_new(elem_size, dtor);
}

void uint_map_drop(UintMap* m) {
    map_drop(m);
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
    return map_find(m, uint_cmp, &key);
}

//
StringMap* string_map_new(size_t elem_size, void (*dtor)(void*)) {
    return map_new(elem_size, dtor);
}

void string_map_drop(StringMap* m) {
    map_drop(m);
}

void* string_map_insert(StringMap* m, char const* key, int* exist) {
    // TODO: improve performance
    void* content = string_map_find(m, key);
    if (content) {
        if (exist) { *exist = 1; };
        return content;
    }

    if (exist) { *exist = 0; };
    Elem* elem = (Elem*)vector_append(m->elems);
    elem->key.name = key;
    elem->buffer = (char*)malloc(m->elem_size); // TODO: consider alignment

    return (void*)elem->buffer;
}

void* string_map_find(StringMap* m, char const* key) {
    return map_find(m, string_cmp, &key);
}
