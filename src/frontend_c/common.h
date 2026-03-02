/* common.h – basic utilities shared by all frontend_c modules */
#pragma once
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ------------------------------------------------------------------ */
/* Memory utilities                                                     */
/* ------------------------------------------------------------------ */
static inline void *xmalloc(size_t n) {
    void *p = malloc(n);
    if (!p) { fprintf(stderr, "fatal: out of memory\n"); exit(1); }
    return p;
}
static inline void *xcalloc(size_t n, size_t sz) {
    void *p = calloc(n, sz);
    if (!p) { fprintf(stderr, "fatal: out of memory\n"); exit(1); }
    return p;
}
static inline void *xrealloc(void *p, size_t n) {
    void *q = realloc(p, n);
    if (!q) { fprintf(stderr, "fatal: out of memory\n"); exit(1); }
    return q;
}
static inline char *xstrdup(const char *s) {
    if (!s) return NULL;
    char *p = strdup(s);
    if (!p) { fprintf(stderr, "fatal: out of memory\n"); exit(1); }
    return p;
}

/* ------------------------------------------------------------------ */
/* Dynamic string buffer (may contain embedded NUL bytes)              */
/* ------------------------------------------------------------------ */
typedef struct {
    char   *buf;
    size_t  len;
    size_t  cap;
} StrBuf;

static inline void sb_init(StrBuf *b) { b->buf = NULL; b->len = 0; b->cap = 0; }

static inline void sb_push(StrBuf *b, int c) {
    if (b->len + 1 >= b->cap) {
        b->cap = b->cap ? b->cap * 2 : 128;
        b->buf = (char *)xrealloc(b->buf, b->cap);
    }
    b->buf[b->len++] = (char)c;
    b->buf[b->len] = '\0';  /* keep NUL-terminated for safety */
}

/* Append a NUL-terminated string */
static inline void sb_puts(StrBuf *b, const char *s) {
    while (*s) sb_push(b, (unsigned char)*s++);
}

/* Return the buffer as a fresh malloc'd string + write length to *out_len.
   The caller owns the returned string.  The StrBuf is reset. */
static inline char *sb_finish(StrBuf *b, size_t *out_len) {
    size_t n = b->len;
    /* Ensure NUL terminator slot exists even for empty buffer */
    char *s;
    if (b->buf) {
        b->buf[n] = '\0';
        s = b->buf;          /* Transfer ownership */
    } else {
        s = (char *)xmalloc(1);
        s[0] = '\0';
    }
    b->buf = NULL; b->len = 0; b->cap = 0;
    if (out_len) *out_len = n;
    return s;
}

/* Convenience: finish and ignore length (for strings without NUL bytes) */
static inline char *sb_str(StrBuf *b) { return sb_finish(b, NULL); }

/* ------------------------------------------------------------------ */
/* Misc macros                                                          */
/* ------------------------------------------------------------------ */
#define ARRAY_LEN(a)  ((int)(sizeof(a) / sizeof((a)[0])))
#define UNUSED(x)     ((void)(x))
