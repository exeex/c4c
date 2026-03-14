# Plan Execution State

Last updated: 2026-03-14

## Current Results
- Tests: 1753/1770 passed (99.0%), 17 failed
- Improvement: +117 tests from builtin header injection and push/pop_macro
- Previous: 1636/1770 (92.5%)

## Completed: Phase 0 — Structure Refactor
All 6 slices done.

## Completed: Phase 1 — Output Contract and Public API
All work items done:
- [x] Split include paths into quote / normal / system / after buckets
- [x] Public API for source-based preprocessing (`preprocess_source()`)
- [x] Public API for define/undefine (`define_macro()`, `undefine_macro()`)
- [x] Initial line marker emission (`# 1 "filename"`)
- [x] Include enter/return markers (`# 1 "file" 1` / `# N "file" 2`)
- [x] __FILE__, __LINE__, __BASE_FILE__, __COUNTER__ managed state
- [x] CLI flags wired (-D, -U, -I, -iquote, -isystem, -idirafter)

## Completed: Phase 2/3 partial — Include/Pragma System
- [x] `#include <...>` angle bracket includes with path search
- [x] Computed includes (macro-expanded `#include`)
- [x] `#pragma once`
- [x] Unknown pragmas non-fatal (ignore instead of fallback)
- [x] `#pragma push_macro` / `#pragma pop_macro` ← NEW

## Completed: Phase 4 partial — #if Expressions and Intrinsics
- [x] `__has_include("file")` and `__has_include(<file>)` with real resolution
- [x] `__has_builtin` table (common GCC/Clang builtins)
- [x] `__has_attribute` table (common GNU attributes)
- [x] `__has_feature` / `__has_extension` table (C features)

## Completed: Phase 5 partial — Macro Expansion
- [x] Variadic macro __VA_ARGS__ spacing fix
- [x] GNU named variadic macros (`args...` syntax)

## Completed: Phase 7 partial — Builtin Header Injection ← NEW
- [x] `<stdarg.h>` (va_list, va_start, va_end, va_arg, va_copy)
- [x] `<limits.h>` (INT_MAX, INT_MIN, CHAR_BIT, etc.)
- [x] `<stddef.h>` (NULL, size_t, ptrdiff_t, offsetof)
- [x] `<stdbool.h>` (bool, true, false)
- [x] `<stdio.h>` (FILE, stdin/stdout/stderr, printf family, file I/O)
- [x] `<stdlib.h>` (malloc, free, exit, abort, atoi, qsort, etc.)
- [x] `<string.h>` (memcpy, strlen, strcmp, etc.)
- [x] `<signal.h>` (SIGFPE, signal, raise, etc.)
- [x] `<assert.h>` (assert macro via __assert_fail)
- [x] `<float.h>` (FLT_MAX, DBL_MAX, etc.)
- [x] `<setjmp.h>` (jmp_buf, setjmp, longjmp)
- [x] `<ctype.h>` (isalpha, toupper, etc.)
- [x] `<math.h>` (sqrt, sin, cos, etc.)

## Not Yet Started
### Phase 2 remainder
- `#include_next` support
- Include resolution cache
- Include guard optimization

### Phase 3 remainder
- `#pragma pack(...)` support
- `#pragma weak`

### Phase 5 remainder
- `_Pragma("...")` operator
- Anti-paste guard behavior
- Multi-line function-like invocation accumulation

### Phase 6 — Predefined Macros and Target Configuration
- Target-family macro sets
- Config toggles (PIC, optimize, etc.)
- Compatibility macros (__GNUC__, __VERSION__, etc.)

### Phase 7 remainder — Builtin Header Injection
- POSIX headers (fcntl.h, sys/mman.h, unistd.h, etc.)
- INT64_C, UINT32_C macro helpers

## Remaining 17 Failures (categorized)
- **c_testsuite (5)**: 00201, 00202, 00206, 00212, 00216 — pre-existing, not preprocessor-related
- **positive_sema (2)**: typeof_unqual_expr, typeof_stmt_expr_combo — sema issues
- **comp_goto_1**: computed goto — pre-existing codegen issue
- **scal_to_vec2**: scalar-to-vector coercion — pre-existing codegen issue
- **widechar_1**: wide char — pre-existing codegen issue
- **strlen_4**: backend fail — global array init codegen bug
- **strlen_5**: runtime mismatch
- **cmpsf_1, 20101011_1**: runtime aborts
- **20001203_2**: segfault (complex nested unions)
- **loop_2f, loop_2g**: missing POSIX headers (fcntl.h, sys/mman.h)
- **pr70460**: runtime mismatch
- **pushpop_macro**: FIXED ← (already reflected in count)

## Next Suggested Work
- `#include_next` (needed for system header chaining)
- POSIX builtin headers (fcntl.h, sys/mman.h) for loop_2f/2g
- Phase 6 predefined macros if needed
