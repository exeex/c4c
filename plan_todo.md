# Plan Todo

Status: Active
Source Idea: ideas/open/03_lir_to_backend_ir_refactor.md
Source Plan: plan.md

## Active Item

- Step 2: continue extracting decode-only helper clusters from
      `lir_adapter.cpp` after the extern-declaration lowering path moved into
      `src/backend/lowering/extern_lowering.cpp`

## Incomplete Items

- [ ] Step 2: separate legacy LIR syntax decode helpers from backend IR
      construction
- [ ] Step 3: convert the highest-value backend IR text-carried semantics into
      structured representations
- [ ] Step 4: remove at least one target-codegen dependency on
      `parse_backend_*` helpers
- [ ] Step 5: run targeted and full regression validation for each landed slice

## Completed Items

- [x] Activated `ideas/open/03_lir_to_backend_ir_refactor.md` into `plan.md`
- [x] Initialized `plan_todo.md` with matching source metadata
- [x] Step 1 slice: expose `lower_to_backend_ir(...)` directly from the
      `lir_adapter` boundary while keeping `adapt_minimal_module(...)` as a
      compatibility shim
- [x] Step 1 slice: move the remaining backend boundary tests onto
      `lower_to_backend_ir(...)`; only the explicit shim-equivalence test still
      calls `adapt_minimal_module(...)`
- [x] Step 2 slice: move the shared typed-call decode helpers out of
      `src/backend/lir_adapter.cpp` into `src/backend/lowering/call_decode.cpp`
      and cover the direct-global vararg prefix path in
      `backend_lir_adapter_tests`
- [x] Step 2 slice: move extern-call parameter inference and extern-declaration
      lowering out of `src/backend/lir_adapter.cpp` into
      `src/backend/lowering/extern_lowering.cpp`, with direct adapter tests for
      inferred fixed params and inconsistent typed-call surfaces

## Next Intended Slice

- Extract the next decode-only utility cluster from `src/backend/lir_adapter.cpp`
  without changing backend IR construction behavior; the likely follow-on is the
  remaining call-normalization path that still couples typed-call parsing to
  adapter-local instruction construction.

## Blockers

- Stable full-suite regression guard still reports the same three pre-existing
  failures:
  `positive_sema_linux_stage2_repro_03_asm_volatile_c`,
  `backend_lir_adapter_aarch64_tests`, and
  `llvm_gcc_c_torture_src_20080502_1_c`.
- `test_fail_before.log` vs `test_fail_after.log` remains monotonic at
  2668 passed / 3 failed / 2671 total with zero newly failing tests.
- Focused `backend_lir_adapter_tests` and `backend_ir_tests` both pass after the
  extern-lowering extraction.

## Resume Notes

- Keep this slice behavior-preserving.
- Do not pull `bir` naming into this plan.
- `lower_to_backend_ir(...)` now lives in `src/backend/lir_adapter.hpp` and
  `src/backend/lir_adapter.cpp`; `adapt_minimal_module(...)` remains a
  compatibility shim for callers that have not moved yet.
- The Step 1 boundary migration is now complete at the direct-call-site level;
  the remaining `adapt_minimal_module(...)` usage is the intentional
  compatibility assertion in `tests/backend/backend_lir_adapter_tests.cpp`.
- The latest full-suite run is recorded in `test_after.log`; the failing tests
  listed above still match the monotonic regression guard baseline from
  `test_fail_before.log` vs `test_fail_after.log`.
- `src/backend/lowering/extern_lowering.cpp` now owns extern-call parameter
  inference and extern-declaration lowering; `src/backend/lir_adapter.cpp`
  only dispatches into that lowering helper for `module.extern_decls`.
- If the work expands into a separate migration initiative, record it under
  `ideas/open/` instead of widening the active runbook.
