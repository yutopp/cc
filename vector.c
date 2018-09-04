#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "vector.h"

struct vector_t {
    size_t elem_size;
    size_t len;
    size_t cap;
    char* buffer;
};

static bool extend(Vector *v, size_t ext_size);

Vector* vector_new(size_t elem_size) {
    Vector* v = (Vector*)malloc(sizeof(Vector));
    if (!v) {
        return 0;
    }
    v->elem_size = elem_size;
    v->len = 0;
    v->cap = 0;
    v->buffer = 0;

    return v;
}

void vector_drop(Vector *v) {
    if (!v) {
        return;
    }

    if (v->buffer) {
        free(v->buffer);
    }

    free(v);
}

void* vector_append(Vector *v) {
    if (v->len == v->cap) {
        size_t ext_size = v->cap == 0 ? 24 : v->cap * 2;
        if (!extend(v, ext_size)) {
            return 0;
        }
    }

    char* p = &v->buffer[v->elem_size * v->len];
    v->len++;

    return p;
}

void* vector_at(Vector *v, size_t index) {
    if (index >= v->len) {
        return 0;
    }

    return &v->buffer[v->elem_size * index];
}

size_t vector_len(Vector *v) {
    return v->len;
}

size_t vector_cap(Vector *v) {
    return v->cap;
}

static bool extend(Vector *v, size_t ext_size) {
    size_t new_cap = v->cap + ext_size;
    char* new_buffer = (char*)malloc(v->elem_size * new_cap);
    if (!new_buffer) {
        return false;
    }

    if (v->buffer) {
        memcpy(new_buffer, v->buffer, v->elem_size * v->len);
    }

    v->cap = new_cap;
    v->buffer = new_buffer;

    return true;
}
