/* ir_builder.h – public interface for the LLVM IR text emitter (Phase 1)
 *
 * Phase-1: string-based LLVM IR output only.
 * Phase-2 (future): LLVM API backend behind a compile-time switch.
 */
#pragma once
#include "ast.h"
#include <stdio.h>

/* Emit a complete LLVM IR translation unit to `out`.
 *
 * `prog`      – NK_PROGRAM root node returned by parse().
 * `module_id` – string used in the `; ModuleID = '...'` comment (e.g. the
 *               input file name).  May be NULL (uses "module").
 * `out`       – destination FILE; may be stdout or an open file.
 */
void ir_emit_program(Node *prog, const char *module_id, FILE *out);
