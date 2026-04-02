# Plan Todo

Status: Active
Source Idea: ideas/open/03_lir_to_backend_ir_refactor.md
Source Plan: plan.md

## Active Item

- Step 2: continue splitting decode-only helpers from `lir_adapter.cpp` after
      the typed-call decode extraction lands

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

## Next Intended Slice

- Extract the next decode-only utility cluster from `src/backend/lir_adapter.cpp`
  without changing backend IR construction behavior; the likely next target is
  the extern-parameter inference / call-normalization path that still depends on
  typed-call parsing.

## Blockers

- Stable full-suite `ctest --test-dir build -j8 --output-on-failure` before and
  after this slice report the same three pre-existing failures:
  `positive_sema_linux_stage2_repro_03_asm_volatile_c`,
  `backend_lir_adapter_aarch64_tests`, and
  `llvm_gcc_c_torture_src_20080502_1_c`.
- Focused `backend_lir_adapter_tests` and `backend_ir_tests` both pass after the
  helper extraction.

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
  listed above match `test_before.log`, so this slice held the suite at the
  same three known failures.
- If the work expands into a separate migration initiative, record it under
  `ideas/open/` instead of widening the active runbook.
