/* parser.h – interface for the recursive-descent C parser */
#pragma once
#include "ast.h"
#include "lexer.h"

/*
 * Parse a token stream produced by lex_source() and return the root NK_PROGRAM
 * node.  All AST nodes are allocated from *arena*.
 *
 * On a hard parse error, prints to stderr and exits.
 */
Node *parse(TokenVec *tv, Arena *arena);
