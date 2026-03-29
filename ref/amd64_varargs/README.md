# AMD64 Varargs Baseline (2026-03-29)

- Host triple: `x86_64-pc-linux-gnu` (`clang -dumpmachine`).
- Baseline commands (run from repo root):
  - `cmake -S . -B build && cmake --build build -j8`
  - `ctest --test-dir build -j --output-on-failure > ref/amd64_varargs/test_before_20260329.log`
  - `ctest --test-dir build -R "(positive_sema_inline_diagnostics|positive_sema_ok_fn_returns_variadic_fn_ptr|va_arg|stdarg|strct_stdarg|scal_to_vec|00174|00204)" -j --output-on-failure > ref/amd64_varargs/ctest_subset_20260329.log`

## Failure Buckets

| Bucket | Representative Test | Failure mode | Notes |
|--------|---------------------|--------------|-------|
| Inline diagnostics runtime shim | `tests/c/internal/positive_case/inline_diagnostics.c` | `C2LL_RUNTIME_UNEXPECTED_RETURN` (segfault) | Artifacts under `inline_diagnostics/`
| Variadic function pointer call | `tests/c/internal/positive_case/ok_fn_returns_variadic_fn_ptr.c` | `C2LL_RUNTIME_UNEXPECTED_RETURN` (segfault) | Artifacts under `variadic_fn_ptr/`
| Glibc macro parser | `tests/c/external/c-testsuite/src/00174.c` | Frontend parse failure in `mathcalls-helper-functions.h` | `c4cll.hir` / `c4cll.ll.err` capture diagnostics because lowering never runs
| C testsuite varargs stress | `tests/c/external/c-testsuite/src/00204.c` | Runtime segfault | Artifacts under `c_testsuite_00204/`
| GCC torture `va_arg_*` (23 cases) | `tests/c/external/gcc_torture/src/va-arg-1.c` | Runtime mismatches vs clang | Artifacts under `gcc_va_arg/`
| GCC torture `stdarg_*` (4 cases) | `tests/c/external/gcc_torture/src/stdarg-1.c` | Runtime mismatches | Artifacts under `gcc_stdarg/`
| GCC torture `strct_stdarg_*` (1 case) | `tests/c/external/gcc_torture/src/strct-stdarg-1.c` | Runtime mismatches | Artifacts under `gcc_strct_stdarg/`
| GCC torture `scal_to_vec*` (2 cases) | `tests/c/external/gcc_torture/src/scal-to-vec1.c` | Runtime mismatches | Artifacts under `gcc_scal_to_vec/`

## Artifact Layout

Each bucket directory contains:

- `c4cll.hir`: `build/c4cll --dump-hir <case>` output (diagnostics for `00174`).
- `c4cll.ll`: LLVM IR emitted by `build/c4cll <case> -o ...` when lowering succeeded.
- `c4cll.ll.err`: present only when lowering failed (see `c_testsuite_00174`).
- `clang.ll`: Reference IR from `clang -S -emit-llvm -O0` with the same language mode (`-std=gnu11` for internal cases, `-std=gnu89 -w -I <suite>` for external suites).

Logs at the root of this directory capture the full suite baseline (`test_before_20260329.log`, 75 failures / 2248 tests) and the targeted subset focus log (`ctest_subset_20260329.log`, 34 failures / 44 tests).
