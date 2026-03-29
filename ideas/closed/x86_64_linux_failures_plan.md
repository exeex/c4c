# x86_64 Linux ABI and Runtime Recovery Plan

Status: Closed  
Last Updated: 2026-03-29  
Source Logs:
- `ref/amd64_varargs/test_before_20260329.log` (x86_64-linux, `ctest` full suite)  
- `ref/amd64_varargs/test_after_20260329_step7.log` (post-recovery run, 2026-03-29)
Suite Snapshot: Baseline 97% pass (75 failed / 2248). After recovery 99% pass (1 failed / 2253, `llvm_gcc_c_torture_src_pr28982b_c`).

## Why This Idea

x86_64-linux runs are still failing in several high-value areas that already
pass on other targets. The current `test_before.log` shows:

- Runtime segfaults in `positive_sema_inline_diagnostics_c` and
  `positive_sema_ok_fn_returns_variadic_fn_ptr_c`
  (`C2LL_RUNTIME_UNEXPECTED_RETURN`, see log lines ~213 and ~285).
- Inline asm rejection in `positive_sema_linux_stage2_repro_03_asm_volatile_c`
  (clang complains about unrecognized mnemonic `yield`, log lines ~230).
- `c_testsuite` failures 00174 and 00204 because libc macros in
  `/usr/include/x86_64-linux-gnu/bits/mathcalls-helper-functions.h` expand into
  syntax our parser rejects (`expected RPAREN`, log lines ~1447).
- 70+ `llvm_gcc_c_torture` varargs/struct-stdarg cases (IDs 2159-2228 and
  neighbors) that segfault at runtime (`clang_exit=0` but `c2ll_exit=Segmentation fault`,
  log lines ~5259), demonstrating that we still violate the SysV AMD64
  variadic calling convention.

This cluster blocks full-suite success on the primary Linux host platform and
prevents downstream validation work that depends on varargs-heavy libc tests.

## Current Failure Shape

### Internal positive cases (3 failures)

| Test | Symptom | Hypothesis |
| --- | --- | --- |
| `positive_sema_inline_diagnostics_c` | Segfault on instrumentation exit | Inline diagnostic trampoline mismanages callee-save state on x86_64 |
| `positive_sema_ok_fn_returns_variadic_fn_ptr_c` | Segfault when invoking returned variadic fn ptr | Missing SysV varargs prologue/epilogue support |
| `positive_sema_linux_stage2_repro_03_asm_volatile_c` | clang compile failure: `invalid instruction mnemonic 'yield'` | Need inline-asm lowering aliasing `yield` -> `pause` |

### Platform-Aware Exclusions

Some existing tests should stay gated to the architectures they were authored
for. We will enforce these via the existing target-triple helpers (e.g.
`target_is_x86_64` / `target_is_aarch64` in `src/frontend/preprocessor/pp_predefined.cpp`)
instead of attempting to “fix” them on x86_64:

- `tests/c/internal/inline_asm/aarch64/simple.c` – only valid on ARM64, so it
  should remain disabled on x86 hosts.
- `tests/c/external/c-testsuite/src/00204.c` – explicitly probes ARM64 ABI/HFA
  behavior and should only run where that ABI matches.

These platform-aware skips are out of scope for the x86_64 recovery work; we
just need to ensure the CMake/test plumbing consults the triple helpers so that
ARM-specific coverage does not block Linux/x86_64 runs.

### c_testsuite (2 failures)

`00174.c` and `00204.c` both include `<math.h>` paths that expand
`__MATHCALL`/`__MATHCALLX` macros. Our parser currently chokes on nested
parameter lists that introduce identifiers like `__value` before closing
parentheses. We need to either predefine the relevant glibc macros or teach
the parser how to accept these helper macros.

### GCC torture (70 failures)

All current `llvm_gcc_c_torture` failures on x86_64-linux are varargs-heavy
(`va_arg_*`, `stdarg_*`, `strct_stdarg_*`, `scal_to_vec*`, etc.) or closely
related runtime ABI tests. Every failure shows `[RUNTIME_FAIL] ... Segmentation
fault` with `clang_exit=0`, indicating code generation rather than frontend
issues. The most likely gap is our incomplete implementation of the SysV AMD64
varargs argument classification, reg-save area, and red zone handling when
bridging to libc helpers.

## Objectives

1. Achieve parity with clang on the AMD64 SysV calling convention for both
   fixed and variadic functions, including register save/restore and stack
   home area layout.
2. Parse and lower glibc header macros used by `mathcalls-helper-functions.h`
   without hand-editing the headers.
3. Allow inline `asm volatile("yield")` by mapping the mnemonic to the correct
   x86_64 instruction sequence (alias for `pause`).
4. Keep regression guard at or below 75 failures during development; end goal
   is zero failures from this cluster and no new regressions.

## Execution Plan

1. **Baseline + Instrumentation**
   - Re-run the failing subset under `ctest -R "(positive_sema_inline_diagnostics|positive_sema_ok_fn_returns_variadic_fn_ptr|va_arg|stdarg|strct_stdarg|scal_to_vec|00174|00204)"`.
   - Capture `c2ll` disassembly and stack traces for representative vararg
     failures to confirm register/stack layout issues.
   - Document clang reference assembly for the same tests.

2. **SysV Varargs Implementation**
   - Implement the AMD64 ABI argument classification tables (int, sse, memory,
     x87) inside the call lowering layer.
   - Ensure the home area/reg-save layout matches `va_list` expectations used
     by glibc macros (`__va_arg_list` structure).
   - Add focused runtime tests that call variadic functions returning structs,
     nested vararg forwarding, and `va_copy`.

3. **Variadic Function Pointer Handling**
   - Fix the call-site lowering when invoking function pointers flagged as
     variadic so they reuse the new SysV varargs path.
   - Update `positive_sema_ok_fn_returns_variadic_fn_ptr_c` to pass locally and
     add an internal regression to guard the behavior.

4. **Inline Diagnostics Runtime Safety**
   - Audit the inline diagnostics instrumentation shim on x86_64 (callee-save
     register preservation, stack alignment, vector register spills).
   - Add a lightweight smoke test that intentionally triggers diagnostics in a
     tight loop to ensure no segfault occurs.

5. **Inline ASM `yield` Alias**
   - Teach the inline-asm validator about architecture-specific pseudo
     instructions; map `yield` → `pause` when targeting x86_64.
   - Cover with a new positive test mirroring
     `positive_sema_linux_stage2_repro_03_asm_volatile_c`.
   - Use the existing target triple helpers (see
     `src/frontend/preprocessor/pp_predefined.cpp:31` for
     `target_is_x86_64`) to keep these fixes platform aware so ARM and other
     hosts stay untouched while we focus on Linux/x86_64.

6. **glibc Macro Parsing Support**
   - Expand the preprocessor to understand the macro patterns used in
     `mathcalls-helper-functions.h` (nested parameter packs, double-parenthesis
     macros).
   - Alternatively, detect when system headers require GNU extensions and
     enable the gnuised mode by default on Linux builds.
   - Verify `c_testsuite` cases 00174 and 00204 compile cleanly afterward.

7. **Validation + Guardrails**
   - Run the targeted subset after each major fix plus a daily full `ctest`.
   - Capture `test_after.log` once all named failures pass; attach both logs to
     the idea for future reference.
   - Keep a running list of any residual failures that fall outside this plan
     and spin them into separate ideas instead of silently growing scope.

## Success Criteria

- `ctest` on x86_64-linux reports zero failures for the named internal,
  `c_testsuite`, and `llvm_gcc_c_torture` cases.
- No new regressions appear elsewhere in the suite (full count stays at or
  below the pre-work failure total of 75).
- Regression coverage added for: variadic function pointer invocation,
  SysV-compliant `va_copy`/`va_arg`, inline diagnostics, and `asm("yield")`.
- Updated documentation/logs (`test_after.log`) stored beside this plan.

## Completion Summary (2026-03-29)

- Captured `ctest` before/after pairs in `ref/amd64_varargs/` and re-ran the named AMD64 subsets while implementing each runbook slice; artifacts include clang/c4cll IR snapshots per failure bucket plus the final `test_after_20260329_step7.log`.
- Implemented AMD64 SysV classification plus register save/restore so both fixed and variadic callees align with glibc `va_list` expectations; gcc torture buckets `va_arg_*`, `stdarg_*`, `strct_stdarg_*`, and `scal_to_vec*` all pass on x86_64-linux.
- Variadic function pointer invocation now reuses the SysV variadic lowering, and `positive_sema_ok_fn_returns_variadic_fn_ptr_c` passes under the focused `ctest -R` guardrail.
- Inline diagnostics trampoline preserves callee-saved registers and stack alignment; stress test `tests/c/internal/positive_case/inline_diagnostics_runtime.c` remains green under AddressSanitizer.
- Inline asm accepts `yield` on x86_64 by aliasing to `pause`, matching clang behavior and unblocking `positive_sema_linux_stage2_repro_03_asm_volatile_c`.
- Glibc macro parsing handles `_Float{16,32,64,128}` helper forms, restoring `c_testsuite` cases 00174 and 00204 plus the reduced repro harness.
- Full-suite regression guard passed: `.codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before --after test_after` reports +79 net passing tests, 74 failures resolved, 0 new failing tests.

## Leftover Issues

- `llvm_gcc_c_torture_src_pr28982b_c` remains the lone x86_64 failure (also failed in the baseline log). The generated IR passes a 256 KiB struct literal to `bar` by value, which triggers an LLVM 19.1 SelectionDAG crash (`clang_exit=0 c2ll_exit=no such file or directory`). We need a follow-on idea to revisit large-by-value aggregate lowering on AMD64: either emit `llvm.memcpy` + `byval` semantics or force memory-class emission to avoid materializing the entire struct in SSA form. Until that follow-up lands this runbook is considered complete with one tracked straggler.
