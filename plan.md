# tiny-c2ll Plan (Frontend-Failure Focus)

Last updated: 2026-03-06

## Current State

- Active frontend is C++ in `src/frontend_c/`.
- Python frontend is removed.
- Core smoke suites pass:
  - `tiny_c2ll_tests`
  - `frontend_cxx_preprocessor_tests`
- `20010605-2.c` frontend fix is completed (`__real__/__imag__` support).

## Allowlist Strategy

- `tests/llvm_gcc_c_torture_allowlist.txt` is generated from:
  - `tests/llvm_gcc_c_torture_frontend_failures.tsv`
- Purpose: focus Claude/Codex loop on known frontend failure set instead of full ~1.7k cases.

## Current First Failure (Focused Run)

Command used:

```bash
PRUNE_FAILED_ALLOWLIST=0 ./scripts/check_progress_llvm_gcc_c_torture.sh
```

Result:
- First fail: `llvm_gcc_c_torture_ieee_pr72824_2_c`
- Failure kind: `[BACKEND_FAIL]`
- Error: invalid LLVM IR cast (`ptrtoint ptr ... to [4 x float]`)
- Classification: frontend IR generation gap (GNU vector extension semantics)

Focused-run snapshot (2026-03-06):
- 46 tests passed before first failure
- `98%` passed in the truncated first-fail run (`1/47` failed)

Recent completed slices now reflected in working tree:
- parser: `typeof(type|identifier)`, `__builtin_types_compatible_p`, `vector_size(...)` parsing hooks
- preprocessor: literal-prefix-safe macro expansion (`L/u/U/u8`), integer limit predefined macros
- IR builder: more builtins (`ffs/clz/ctz/popcount/parity`, `copysign`, `nan`, `is*`), complex compound assignment/conjugate, safer `void*` GEP element typing
- allowlist: commented known non-frontend blockers (`bcp-1.c`, `comp-goto-1.c`)

Latest slice (2026-03-06, follow-up handoff):
- parser: added `NK_OFFSETOF` support path in `parse_primary()` for `__builtin_offsetof(type, member[.submember])`
- parser: wired struct definition map updates so parse-time constant evaluation can resolve struct field offsets
- note: focused allowlist is kept in the frontend-failure list form (not pruned-to-only-last-fail form)

Latest slice (2026-03-06, vector array/decay follow-up — 100/100 allowlist pass):
- parser/ast: track typedef-inner array rank (`inner_rank`) and vector-origin arrays (`is_vector`) to distinguish true arrays vs GNU vector-like aggregates
- parser: accept GNU `__extension__` unary form
- IR builder: improve array-of-pointer/typedef-array lowering, builtin remapping (`__builtin_*` ↔ libc), vector/array assignment+return materialization, and commutative indexing (`n[p]`)
- allowlist: commented `pr22061-1.c` as current non-frontend blocker on macOS runtime (`_alloca`) plus missing VLA stride support
- focused run status after fixes: **100/100 allowlist tests pass**
  - ieee/pr72824-2.c: fixed (array-of-vectors NK_VAR decay)
  - pr53645-2.c: fixed (vec_val double-load for NK_DEREF/NK_VAR of pure vector)
  - pr85169.c: fixed (array assignment double-load for pure vector RHS)
  - strlen-4.c: fixed (multi-dim array decay missing is_ptr_to_array in expr_type)

Action policy:
1. All 100 focused allowlist cases now pass.
2. Next step: expand allowlist or run full llvm_gcc_c_torture suite to find next failures.

## Active Repair Workflow

1. Run first-fail loop:
   - `PRUNE_FAILED_ALLOWLIST=0 ./scripts/check_progress_llvm_gcc_c_torture.sh`
2. If fail is non-frontend (`CLANG_COMPILE_FAIL`, platform-link `BACKEND_FAIL`):
   - comment out case in `tests/llvm_gcc_c_torture_allowlist.txt`
   - rerun.
3. If fail is frontend (`FRONTEND_FAIL`, `FRONTEND_TIMEOUT`):
   - fix smallest root cause in parser/typing/IR.
4. Commit one small slice each iteration.

## Execution Rules

1. One smallest fix slice per commit.
2. Re-run first-fail loop after each slice.
3. Do not edit vendored test-suite sources:
   - `tests/c-testsuite/`
   - `tests/llvm-test-suite/`
4. If blocked >15 min, log blocker in `build/agent_state/hard_bugs.md` and switch hypothesis.

## Useful Commands

```bash
cmake -S . -B build_debug
cmake --build build_debug -j8

PRUNE_FAILED_ALLOWLIST=0 ./scripts/check_progress_llvm_gcc_c_torture.sh

ctest --test-dir build_debug --output-on-failure -R '^llvm_gcc_c_torture_20010122_1_c$' -j 1
```

## Linux Stage2 Blockers (Ubuntu, 2026-03-06)

Context:
- `CC=/workspaces/compiler/build_linux/tiny-c2ll-stage2`
- Kbuild/assembler probe is now OK.
- Current stop is frontend parse failures on preprocessed Linux sources.

### Repro Setup (shared)

Source file:
- `tests/linux/arch/arm64/kernel/vdso/vgettimeofday.c`

Shared preprocess command:

```bash
clang -E -P -x c \
  -nostdinc \
  -Itests/linux/arch/arm64/include \
  -Itests/linux-build/arch/arm64/include/generated \
  -Itests/linux/include \
  -Itests/linux-build/include \
  -Itests/linux/arch/arm64/include/uapi \
  -Itests/linux-build/arch/arm64/include/generated/uapi \
  -Itests/linux/include/uapi \
  -Itests/linux-build/include/generated/uapi \
  -include tests/linux/include/linux/compiler-version.h \
  -include tests/linux/include/linux/kconfig.h \
  -include tests/linux/include/linux/compiler_types.h \
  -D__KERNEL__ --target=aarch64-linux-gnu -fintegrated-as -mlittle-endian \
  -std=gnu11 -fshort-wchar -funsigned-char -fno-common -fno-PIE -fno-strict-aliasing \
  -ffixed-x18 -DBUILD_VDSO -O2 -mcmodel=tiny \
  -include tests/linux/lib/vdso/gettimeofday.c \
  -Itests/linux/arch/arm64/kernel/vdso -Itests/linux-build/arch/arm64/kernel/vdso \
  tests/linux/arch/arm64/kernel/vdso/vgettimeofday.c \
  -o /tmp/vgettimeofday.i
```

Shared frontend compile command:

```bash
./build_linux/tiny-c2ll-stage1 /tmp/vgettimeofday.i -o /tmp/vgettimeofday.ll 2>/tmp/vgettimeofday.err || true
```

### 4 Concrete Failing Cases

1. `__typeof_unqual__` cast expression fails.
   - File: `tests/linux/arch/arm64/kernel/vdso/vgettimeofday.c` (preprocessed `/tmp/vgettimeofday.i`)
   - Check:
   ```bash
   grep -n "expected RPAREN but got 'volatile' at line 275" /tmp/vgettimeofday.err
   ```

2. GNU statement-expression + inline asm expansion region fails.
   - File: `tests/linux/arch/arm64/kernel/vdso/vgettimeofday.c` (preprocessed `/tmp/vgettimeofday.i`)
   - Check:
   ```bash
   grep -n "expected RPAREN but got '\\*' at line 812" /tmp/vgettimeofday.err
   ```

3. `typeof(...)` + `sizeof(...)` heavy macro expansion fails.
   - File: `tests/linux/arch/arm64/kernel/vdso/vgettimeofday.c` (preprocessed `/tmp/vgettimeofday.i`)
   - Check:
   ```bash
   grep -n "expected RPAREN but got 'sizeof' at line 2252" /tmp/vgettimeofday.err
   ```

4. Instrumentation macro expression path fails (constant/paren-rich call site).
   - File: `tests/linux/arch/arm64/kernel/vdso/vgettimeofday.c` (preprocessed `/tmp/vgettimeofday.i`)
   - Check:
   ```bash
   grep -n "expected RPAREN but got '0' at line 3630" /tmp/vgettimeofday.err
   ```

Action next:
1. Add parser coverage for `__typeof_unqual__(expr)` and qualifier-bearing GNU `typeof` forms.
2. Re-test same file and confirm line-275 family disappears before tackling downstream cascades.
