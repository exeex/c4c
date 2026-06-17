# RV64 Runtime Defined Global Scalar Memory Foundation

## Goal

Extend the qemu-backed RV64 runtime path to cover simple defined global scalar
memory: loading an initialized `int`, loading a zero-initialized `int`, and
storing then reloading an `int` global.

This is the next capability step after the RV64 prepared emitter decomposition
and context cleanup. It should add semantic global-memory lowering rules, not
testcase-shaped shortcuts.

## Why This Exists

The current RV64 runtime suite covers:

- scalar returns and integer arithmetic;
- branch comparisons;
- local stack memory and local pointer/member indexing;
- direct fixed-arity scalar calls;
- qemu-backed execution with BIR/LLVM fallback rejection.

Recent cleanup ideas also moved the prepared emitter into functional owner
files and added a prepared emission context, so the backend is ready for the
next small feature island without piling more code into `emit.cpp`.

The largest remaining obvious gap for small C runtime cases is global memory.
Start with defined scalar globals before touching arrays, aggregate globals,
global pointer initializers, extern globals, GOT-required forms, or indirect
calls through globals.

## Candidate Runtime Cases

Start from existing `tests/backend/case/` files:

1. `global_load.c`
   - initialized scalar global load;
   - expected exit code `11`.
2. `global_load_zero_init.c`
   - zero-initialized scalar global load;
   - expected exit code `0`.
3. `global_store.c`
   - scalar global store followed by reload;
   - expected exit code `7`.

Register accepted cases with `c4c_add_backend_rv64_runtime_case(...)` only when
they execute through the real RV64 asm route under qemu.

## In Scope

- Emit a minimal RV64 assembly data/bss representation for defined scalar i32
  globals in prepared modules.
- Lower prepared global i32 loads through a real symbol-address rule such as
  `%pcrel_hi/%pcrel_lo` or an equivalent assembler-supported RV64 sequence.
- Lower prepared global i32 stores through the same global-symbol address
  authority.
- Keep the implementation in owner files that match the 301/302 layout, for
  example a prepared global-memory owner or narrowly shared frame/context
  support if needed.
- Preserve fail-closed behavior for unsupported global forms.
- Keep qemu runtime tests rejecting BIR/LLVM fallback output.

## Out of Scope

- Global arrays, nested aggregates, aggregate global stores, string globals, or
  pointer-valued global initializers.
- Extern globals, TLS, GOT/PLT-specific global addressing, or relocation-heavy
  object emission work beyond what clang/as/ld already accepts for the emitted
  assembly.
- Function-pointer globals or indirect calls.
- Varargs, stack arguments, aggregate ABI, dynamic stack, or by-value
  aggregate call/return support.
- Changing the RV64 runtime test runner contract.
- Fixing the known `backend_riscv_prepared_edge_publication` baseline failure
  unless a separate idea owns that repair.

## Suggested Execution Order

1. Inspect the prepared BIR emitted for the three candidate cases and identify
   the exact `Global`, `LoadGlobalInst`, `StoreGlobalInst`, and prepared memory
   metadata already available.
2. Add the smallest owner surface needed for prepared RV64 global memory, rather
   than adding this logic to `emit.cpp` or scalar/local-memory owners by
   accident.
3. Emit defined i32 global data for initialized and zero-initialized globals.
4. Implement i32 global load lowering and register `global_load.c`.
5. Implement zero-initialized global lowering and register
   `global_load_zero_init.c`.
6. Implement i32 global store lowering and register `global_store.c`.
7. Run RV64 runtime validation after each accepted case.
8. Run focused RISC-V and backend-wide validation before closure.

If a candidate requires array layout, pointer initializers, GOT semantics, or
object-linker work larger than scalar defined globals, stop and open a separate
idea instead of widening this one.

## Acceptance Criteria

- At least `backend_rv64_runtime_global_load` runs under qemu with the expected
  exit code.
- Preferably all three candidate cases run under qemu:
  `global_load`, `global_load_zero_init`, and `global_store`.
- The implementation uses global-symbol lowering rules, not named-testcase or
  rendered-text shortcuts.
- `emit.cpp` remains thin and does not regain prepared lowering ownership.
- Unsupported global forms still fail closed instead of falling back to BIR or
  LLVM text.
- `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
  passes with the newly registered global cases.
- `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
  either passes or preserves only documented baseline failures.
- `ctest --test-dir build -R '^backend_' --output-on-failure` is run before
  closure and preserves the accepted baseline.

## Reviewer Reject Signals

- The implementation recognizes `global_load.c`, `global_store.c`, or specific
  function/global names instead of lowering BIR/prepared global semantics.
- The route accepts BIR/LLVM fallback as a runtime success.
- Global arrays, aggregate globals, pointer initializers, indirect calls, or
  GOT-heavy behavior sneak into this scalar foundation.
- `emit.cpp` starts growing prepared global lowering logic.
- Tests or expectations are weakened to claim support.

## Notes For The Agent

- Use the post-302 owner layout as the starting point. Add a small prepared
  global-memory owner if that is the cleanest place for the feature.
- Keep every registered runtime case backed by qemu execution.
- It is acceptable to close with only the first one or two candidate cases if
  the third exposes a separate capability boundary, but document that boundary
  clearly in the closure note.
