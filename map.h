#ifndef CC_MAP_H
#define CC_MAP_H

#include <stddef.h>

struct map_t;
typedef struct map_t UintMap;
typedef struct map_t StringMap;

UintMap* uint_map_new(size_t elem_size, void (*dtor)(void*));
void uint_map_drop(UintMap* m);

void* uint_map_insert(UintMap* m, size_t key, int* found);
void* uint_map_find(UintMap* m, size_t key);

#endif /* CC_MAP_H */
