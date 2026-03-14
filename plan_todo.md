# Plan Execution State

Last updated: 2026-03-14

## Current Results
- Tests: 1774/1774 passed (100%), 0 failed
- Improvement: +10 tests across sessions (from 1764)
- Previous: 1773/1773 (100%)

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
- [x] `#pragma push_macro` / `#pragma pop_macro`

## Completed: Phase 4 partial — #if Expressions and Intrinsics
- [x] `__has_include("file")` and `__has_include(<file>)` with real resolution
- [x] `__has_builtin` table (common GCC/Clang builtins)
- [x] `__has_attribute` table (common GNU attributes)
- [x] `__has_feature` / `__has_extension` table (C features)
- [x] Wide/prefixed char literals (`L'\400'`) in `#if` expressions ← NEW
- [x] Multi-digit octal and hex escapes in `#if` char literals ← NEW

## Completed: Phase 5 partial — Macro Expansion
- [x] Variadic macro __VA_ARGS__ spacing fix
- [x] GNU named variadic macros (`args...` syntax)
- [x] PP-number suffix protection (float suffixes F/L not expanded as macros)

## Completed: Phase 6 partial — Predefined Macros and Target Configuration
- [x] GCC compatibility macros (__GNUC__, __GNUC_MINOR__, __VERSION__, __STDC_HOSTED__)
- [x] Target architecture macros (__aarch64__, __x86_64__, __i386__, __riscv)
- [x] OS macros (__linux__, __unix__, __APPLE__, _WIN32, __ELF__)

## Completed: Phase 7 partial — Builtin Header Injection
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
- [x] `<fcntl.h>` (O_RDONLY, open, etc.)
- [x] `<sys/mman.h>` (mmap, PROT_READ, MAP_PRIVATE, etc.)
- [x] `<sys/types.h>` (size_t, pid_t, off_t, etc.)
- [x] `<sys/stat.h>` (chmod, stat, mode macros)
- [x] `<unistd.h>` (read, write, close, fork, etc.)

## Non-preprocessor Fixes (cumulative)
- [x] Enum scope leak: inner block enum constants no longer leak to outer scope
- [x] `__typeof_unqual__` / `typeof_unqual` keyword support ← NEW
- [x] Statement expressions in ternary branches: side effects now conditional ← NEW
- [x] Ptr-to-array global init type/initializer ← DONE
- [x] 3D+ array initializer: shift array_dims when dropping outer dimension ← NEW
- [x] Ptr-to-array pointer arithmetic: correct GEP stride via llvm_alloca_ty ← NEW
- [x] Ptr-to-array deref: array decay instead of scalar load ← NEW
- [x] AddrOf array: preserve array dims and set is_ptr_to_array ← NEW
- [x] Local multi-dim array init: VLA false positive fix, 2D+ init lists, dim shift ← NEW
- [x] void* pointer subtraction: byte-granular (GCC extension, sizeof(void)==1) ← NEW
- [x] blockaddress constant exprs in static initializers (&&lab1 - &&lab0) ← NEW

## Not Yet Started
### Phase 2 remainder
- [x] `#include_next` support (done in b957e0e)
- [x] Include resolution cache ← NEW
- [x] Include guard optimization ← NEW

### Phase 3 remainder
- [x] `#pragma pack(...)` support — full pipeline: preprocessor→lexer→parser→HIR layout ← NEW
- `#pragma weak`

### Phase 5 remainder
- [x] `_Pragma("...")` operator ← NEW
- [x] Anti-paste guard behavior ← NEW
- [x] Multi-line function-like invocation accumulation ← NEW

### Phase 6 remainder
- [x] aarch64 feature macros (__ARM_NEON, __ARM_FP, __ARM_FEATURE_*, __ARM_PCS_AAPCS64, etc.) ← NEW
- [x] Common GCC compat macros (__ATOMIC_*, __GCC_HAVE_SYNC_*, __BIGGEST_ALIGNMENT__, __FINITE_MATH_ONLY__, __NO_INLINE__, __GNUC_STDC_INLINE__, __STDC_UTF_*, __USER_LABEL_PREFIX__, __GCC_ASM_FLAG_OUTPUTS__) ← NEW
- [x] x86_64 feature macros (__SSE__, __SSE2__, __MMX__, __SSE_MATH__, __SSE2_MATH__, __NO_MATH_INLINES, __SEG_FS/GS, __GCC_HAVE_DWARF2_CFI_ASM, __REGISTER_PREFIX__) ← NEW
- [x] i386 feature macros (__code_model_32__, __NO_MATH_INLINES, __REGISTER_PREFIX__) ← NEW
- [x] Config toggles: -O0/-O1/-O2/-O3/-Os, -fPIC/-fpic, -fPIE/-fpie → __OPTIMIZE__, __OPTIMIZE_SIZE__, __NO_INLINE__, __PIC__/__pic__, __PIE__/__pie__ ← NEW

### Phase 7 remainder
- More POSIX headers as needed
- [x] `<stdint.h>` builtin header (INT64_C, UINT32_C, INT8_C, etc.) ← NEW
- [x] `<inttypes.h>` builtin header (PRId64, PRIu32, imaxabs, etc.) ← NEW
- [x] `<errno.h>` builtin header (errno, EINVAL, ENOENT, etc.) ← NEW
- [x] `<time.h>` builtin header (time_t, clock_t, struct tm, time, clock, etc.) ← NEW
- [x] `__has_include` now checks builtin headers (was missing) ← NEW

## Remaining Failures
- None! All 1773 tests pass.

## Non-preprocessor Fixes (this session)
- [x] `__attribute__((vector_size(N)))` parsing: fixed `skip_attributes()` in `parse_stmt()` and `parse_global_decl_or_function()` discarding type-affecting attributes before parse_base_type could capture them
- [x] Vector init list lowering: added vector path to `consume_from_list` in ast_to_hir.cpp
- [x] Scalar-to-vector binary op result type: update `res_spec` after splatting in hir_emitter.cpp
- [x] `__attribute__((packed))` on struct types: recognized in parse_attributes, sets pack_align=1 ← NEW

## Next Suggested Work
- `#pragma weak` (Phase 3, low priority)
- `__VA_OPT__` support (C2x feature, not yet in plan)
