# Plan Todo

Status: Active
Source Idea: ideas/open/03_lir_to_backend_ir_refactor.md
Source Plan: plan.md

## Active Item

- Step 5: keep the regression guard current for each landed refactor slice
      while selecting the next highest-value backend-owned semantic surface to
      structure beyond direct-call callee metadata and global storage/init
      metadata

## Incomplete Items

- [ ] Step 2: separate legacy LIR syntax decode helpers from backend IR
      construction
- [ ] Step 3: convert the highest-value backend IR text-carried semantics into
      structured representations
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
- [x] Step 2 slice: move the shared typed-call operand matcher wrappers out of
      `src/backend/lir_adapter.hpp` into
      `src/backend/lowering/call_decode.hpp`, and cover structured local-call
      helper decoding directly in `backend_lir_adapter_tests`
- [x] Step 2 slice: move the zero-add local-slot rewrite matcher out of
      `src/backend/lir_adapter.cpp` into
      `src/backend/lowering/call_decode.hpp`, and cover both reload and
      non-reload compatibility shapes directly in `backend_lir_adapter_tests`
- [x] Step 2 slice: move the remaining single-slot versus two-slot local-call
      typed operand recognizers out of `src/backend/lir_adapter.cpp` into
      `src/backend/lowering/call_decode.hpp`, and cover direct-load plus
      rewritten compatibility shapes in `backend_lir_adapter_tests`
- [x] Step 3 slice: convert `BackendCallInst` callee identity from raw
      backend-text into structured backend-owned call-target metadata
      (`DirectGlobal` / `DirectIntrinsic` / `Indirect`) while preserving
      backend IR printing and validation behavior
- [x] Step 3 slice: convert `BackendGlobal` storage and initializer semantics
      from raw qualifier/init text into structured backend-owned metadata
      (`Mutable` / `Constant` plus `Declaration` / `Zero` /
      `IntegerLiteral` / `RawText`) while preserving lowered IR printing and
      validation behavior through compatibility shims for backend consumers
- [x] Step 4 slice: switch the x86 and aarch64 minimal direct-call fast paths
      from reparsing backend call text through
      `parse_backend_direct_global_*` to reading structured
      `BackendCallInst::callee` metadata plus typed-call operands directly
- [x] Step 4 slice: remove the remaining
      `parse_backend_direct_global_*` emitter dependencies from
      `src/backend/x86/codegen/emit.cpp` and
      `src/backend/aarch64/codegen/emit.cpp` by moving the x86 LIR-side
      direct-call recognizers onto shared LIR parsing and keeping the aarch64
      backend matcher on structured `BackendCallInst::callee` metadata plus
      borrowed typed-call operands
- [x] Step 4 slice: remove the remaining emitter-side
      `parse_backend_typed_call(...)` uses from
      `src/backend/x86/codegen/emit.cpp` and
      `src/backend/aarch64/codegen/emit.cpp` by switching those backend-IR
      direct-call matchers onto the narrower direct-global helpers in
      `src/backend/lowering/call_decode.cpp`, with a direct helper regression
      test for the lowered vararg-prefix shape in `backend_lir_adapter_tests`

## Next Intended Slice

- Return to Step 3 and convert the next backend IR semantic surface that still
  relies on text-shaped payloads, likely starting with load/store or pointer-
  difference metadata that currently still leans on string-carried type text,
  while keeping the emitter-side direct-call boundary and new global metadata
  stable.

## Blockers

- Stable full-suite regression guard still reports the same three pre-existing
  failures:
  `positive_sema_linux_stage2_repro_03_asm_volatile_c`,
  `backend_lir_adapter_aarch64_tests`, and
  `llvm_gcc_c_torture_src_20080502_1_c`.
- The latest full-suite run in `test_after.log` remains monotonic with the
  recorded baseline at 2668 passed / 3 failed / 2671 total and zero newly
  failing tests.
- The latest regression-guard check
  (`check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`)
  passes after the `BackendGlobal` metadata slice with the same
  2668 passed / 3 failed / 2671 total baseline and zero new failures.
- Focused `backend_lir_adapter_x86_64_tests` passes after the emitter cleanup;
  focused `backend_lir_adapter_aarch64_tests` still fails in the same
  pre-existing suite called out in the full-suite baseline.
- The latest `ctest --test-dir build -j --output-on-failure > test_after.log`
  run remains monotonic at 2668 passed / 3 failed / 2671 total with the same
  three pre-existing failures recorded in `test_fail_before.log` and
  `test_fail_after.log`.
- Focused `backend_lir_adapter_tests` passes with the new structured
  direct-global helper coverage for the lowered vararg-prefix backend-call
  shape.

## Resume Notes

- Keep this slice behavior-preserving.
- Do not pull `bir` naming into this plan.
- This iteration kept helper ownership only: the local typed-call shape
  recognizers that choose direct-load versus rewritten single-slot and
  two-slot operands now live in `src/backend/lowering/call_decode.hpp`, with
  adapter/emitter behavior unchanged.
- `src/backend/lowering/call_decode.hpp` now owns both the direct-global and
  local typed-call operand matcher templates plus the remaining local-call
  shape recognizers; `src/backend/lir_adapter.hpp` only keeps the public
  lowering entrypoints and adapter error surface.
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
- `BackendCallInst` now stores structured call-target metadata in
  `callee.kind` plus `symbol_name`/`operand` instead of carrying that call
  identity only as raw text; `src/backend/ir_printer.cpp` reconstructs the
  printable LIR-style call operand from that structured form.
- `BackendGlobal` now stores structured storage/init metadata in
  `storage` plus `initializer.kind`, while `qualifier` / `init_text` /
  `is_extern_decl` remain compatibility shims for backend consumers that have
  not migrated yet.
- The x86 and aarch64 minimal direct-call fast paths touched in this slice now
  read structured `BackendCallInst::callee` metadata directly through the
  narrower direct-global helper surface; they no longer depend on generic
  backend call parsing in emitter code.
- `src/backend/x86/codegen/emit.cpp` no longer depends on
  `parse_backend_typed_call(...)` at all; its LIR-only fast-path recognizers
  now use shared `codegen::lir` direct-global typed-call parsing, while the
  backend-IR direct-call helpers use `parse_backend_direct_global_*`.
- `src/backend/aarch64/codegen/emit.cpp` now matches the same direct-global
  helper boundary for backend-IR direct-call slices, including the folded
  two-argument helper path.
- If the work expands into a separate migration initiative, record it under
  `ideas/open/` instead of widening the active runbook.
