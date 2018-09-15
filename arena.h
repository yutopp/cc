#ifndef CC_ARENA_H
#define CC_ARENA_H

struct arena_t;
typedef struct arena_t Arena;

Arena* arena_new(size_t elem_size, void (*dtor)(void*));
void* arena_malloc(Arena *arena);
void arena_drop(Arena *arena);

#endif /* CC_ARENA_H */
