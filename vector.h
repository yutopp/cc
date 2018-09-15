#ifndef CC_VECTOR_H
#define CC_VECTOR_H

#include <stddef.h>

struct vector_t;
typedef struct vector_t Vector;

Vector* vector_new(size_t elem_size);
Vector* vector_new_with_cap(size_t elem_size, size_t cap);
void vector_drop(Vector *vector);

void* vector_append(Vector *vector);
void* vector_at(Vector *vector, size_t index);
size_t vector_len(Vector *vector);
size_t vector_cap(Vector *vector);

#endif /* CC_VECTOR_H */
