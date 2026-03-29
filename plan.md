# AMD64 Linux Recovery Runbook

Status: Active
Source Idea: ideas/open/x86_64_linux_failures_plan.md
Last Updated: 2026-03-29

## Purpose
Restore x86_64-linux parity with clang across inline diagnostics, inline asm, glibc macro parsing, and SysV variadic ABI behaviors so the full suite can run cleanly on the primary host.

## One-Sentence Goal
Drive the failure count from the current 75 down to zero for the named internal, c_testsuite, and gcc torture cases without regressing other platforms.

## Core Rule
Treat clang -S/-emit-llvm output on x86_64 Linux as the reference for ABI shape, argument homes, and inline asm mnemonics; do not generalize changes beyond the AMD64 SysV path unless a regression demands it.

## Read First
- prompts/AGENT_PROMPT_EXECUTE_PLAN.md
- tests/c/internal/positive_sema_* for repro harnesses
- tests/c/external/c-testsuite/src/00174.c, 00204.c
- tests/c/external/llvm_gcc_c_torture/** for vararg regressions

## Scope
- SysV AMD64 variadic call lowering (fixed + variadic callee and caller)
- Variadic function pointer invocation safety
- Inline diagnostics trampoline stability on Linux/x86_64
- Inline asm alias support for `yield` -> `pause`
- Parser/preprocessor support for glibc math helper macros

## Non-Goals
- ARM/AArch64 feature work (keep existing skips)
- Broad refactors to the preprocessor beyond what is needed for glibc headers
- Runtime ABI work unrelated to SysV AMD64 variadics

## Execution Rules
- Add or adjust tests before changing code for each slice (internal positive tests or focused torture filters).
- Every ABI change must be validated against clang IR plus a runtime call/return check using `ctest -R` on the affected case.
- Keep `test_before.log`/`test_after.log` pairs attached to this runbook directory when significant milestones are reached.

## Steps

### 1. Baseline & Instrumentation
- Reproduce named failures via `ctest -R "(positive_sema_inline_diagnostics|positive_sema_ok_fn_returns_variadic_fn_ptr|va_arg|stdarg|strct_stdarg|scal_to_vec|00174|00204)"`.
- Capture `build/c4cll --dump-hir` and clang `-S -emit-llvm` output for one representative from each failure bucket.
- Store the artifacts under `ref/amd64_varargs/` for comparison.
- Completion: Reference artifacts checked in and `plan_todo.md` documents key deltas for each bucket.

### 2. Implement SysV Varargs Classification & Homes
- Extend `src/codegen/llvm/calling_convention.cpp` (or current lowering file) with AMD64 classes (INTEGER, SSE, X87, MEMORY) and reg-save layout that matches glibc's `va_list` expectations.
- Teach callee prologue/epilogue to spill GP/XMM argument registers into the va_list save area and honor the red zone rules.
- Add targeted tests in `tests/c/internal/abi/` covering nested `va_start`/`va_copy`, structs returned from variadic callees, and forwarding wrappers.
- Completion: New tests pass; gcc torture vararg sample set (pick 5) no longer segfaults.

### 3. Variadic Function Pointer Calls
- Ensure call lowering reuses the SysV variadic path whenever `FunctionType::is_variadic()` or metadata indicates a variadic function pointer.
- Extend `positive_sema_ok_fn_returns_variadic_fn_ptr_c` (or add sibling) so it calls through a saved variadic pointer with mixed GP/SSE args.
- Completion: Test case passes under `ctest -R positive_sema_ok_fn_returns_variadic_fn_ptr` and no new regressions in nearby sema tests.

### 4. Inline Diagnostics Shim Stability
- Audit `src/runtime/inline_diagnostics/*.S` (or C shim) to preserve callee-save regs and maintain 16-byte stack alignment on AMD64.
- Add a regression test that intentionally emits inline diagnostics in a tight loop (maybe `tests/c/internal/diagnostics/inline_runtime.c`).
- Completion: `positive_sema_inline_diagnostics_c` passes and the new stress test stays stable under AddressSanitizer locally.

### 5. Inline ASM `yield` Alias
- Update the inline asm validator/parser (likely `src/frontend/sema/sema_asm.cpp`) to treat `yield` as an alias for `pause` when `target_is_x86_64`.
- Mirror the behavior in the actual emitter if needed so generated IR still calls `llvm.x86.sse2.pause`.
- Add/refresh `positive_sema_linux_stage2_repro_03_asm_volatile_c` to assert `yield` compiles.
- Completion: clang compilation no longer fails on the test, and we document the alias in code comments.

### 6. Glibc Macro Parsing Support
- Teach the parser or preprocessor to handle nested parameter macro helpers from `mathcalls-helper-functions.h` (e.g., allow identifiers before closing parens in nested prototypes) or predefine the glibc helper macros when targeting Linux.
- Focus implementation in `src/frontend/preprocessor/pp_macros.cpp` or parser entry point; avoid touching unrelated grammar.
- Validate against `c_testsuite` cases 00174 and 00204 plus a reduced repro extracted into `tests/c/preprocessor/glibc_mathcalls.c`.
- Completion: Both c_testsuite cases compile and run; repro test locked in.

### 7. Validation & Regression Guard
- After each major step re-run the focused subset; once Steps 1–6 pass individually, run full `ctest -j` and record `test_after.log`.
- Compare counts to the initial snapshot (75 failures) and ensure no new failures appear.
- Commit both logs into `ref/amd64_varargs/` for traceability and update the source idea with final numbers before closing.
- Completion: Full suite passes or remaining failures are documented and triaged into new ideas.
