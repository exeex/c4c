/* arena.c – bump-pointer arena allocator implementation */

#include "common.h"
#include "arena.h"
#include <string.h>

/* Access the data region that follows an ArenaBlock header */
static inline char *blk_data(ArenaBlock *b) { return (char *)(b + 1); }

/* Allocate a fresh block of at least `cap` usable bytes */
static ArenaBlock *new_block(size_t cap) {
    ArenaBlock *b = (ArenaBlock *)xmalloc(sizeof(ArenaBlock) + cap);
    b->next = NULL;
    b->cap  = cap;
    b->used = 0;
    /* Zero-initialise the data region so arena_alloc clients get clean memory */
    memset(blk_data(b), 0, cap);
    return b;
}

void arena_init(Arena *a, size_t block_sz) {
    a->head     = NULL;
    a->block_sz = block_sz ? block_sz : 65536u;
}

void *arena_alloc(Arena *a, size_t sz) {
    /* Align to 8 bytes */
    sz = (sz + 7u) & ~(size_t)7u;
    if (sz == 0) sz = 8;

    if (!a->head || a->head->used + sz > a->head->cap) {
        size_t cap = (sz > a->block_sz) ? sz : a->block_sz;
        ArenaBlock *b = new_block(cap);
        b->next  = a->head;
        a->head  = b;
    }

    void *ptr = blk_data(a->head) + a->head->used;
    a->head->used += sz;
    return ptr;
}

char *arena_strdup(Arena *a, const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char  *p = (char *)arena_alloc(a, n);
    memcpy(p, s, n);
    return p;
}

void arena_free_all(Arena *a) {
    ArenaBlock *b = a->head;
    while (b) {
        ArenaBlock *nxt = b->next;
        free(b);
        b = nxt;
    }
    a->head = NULL;
}
