/* arena.h – bump-pointer arena allocator
 *
 * Allocates from large blocks; frees everything at once with arena_free_all().
 * No individual frees supported.  Ideal for AST nodes.
 */
#pragma once
#include <stddef.h>

/* Forward-declared as opaque header + trailing data block */
typedef struct ArenaBlock {
    struct ArenaBlock *next;
    size_t cap;
    size_t used;
    /* data follows immediately after this header in memory */
} ArenaBlock;

typedef struct {
    ArenaBlock *head;
    size_t      block_sz;   /* minimum new-block size */
} Arena;

void  arena_init    (Arena *a, size_t block_sz);
void *arena_alloc   (Arena *a, size_t sz);
char *arena_strdup  (Arena *a, const char *s);
void  arena_free_all(Arena *a);

/* Allocate a zero-initialised array of n elements of size esz */
static inline void *arena_calloc(Arena *a, size_t n, size_t esz) {
    void *p = arena_alloc(a, n * esz);
    /* arena_alloc already zero-initialises (see arena.c) */
    (void)n; (void)esz;
    return p;
}
