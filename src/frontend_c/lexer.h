/* lexer.h – interface for the C frontend lexer */
#pragma once
#include "token.h"
#include <stddef.h>

/* Dynamic array of Tokens */
typedef struct {
    Token  *tokens;
    size_t  count;
    size_t  cap;
} TokenVec;

/*
 * Tokenise `len` bytes of C source from `src`.
 * All token texts are heap-allocated and owned by the returned TokenVec.
 *
 * The caller must free with tokenvec_free() when done.
 *
 * On a hard lexer error the function prints to stderr and exits.
 */
TokenVec lex_source(const char *src, size_t len);

/* Release all memory owned by `tv` (tokens array + all token texts). */
void tokenvec_free(TokenVec *tv);
