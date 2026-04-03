# BIR Full Coverage and Legacy IR Removal

Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md

## Purpose

Eliminate both legacy backend IR and every remaining LLVM rescue path so
backend codegen stands on one canonical BIR-owned route.

## Goal

Expand BIR to full production coverage, migrate emitters to consume BIR
directly, remove legacy backend IR, and remove app-layer LLVM fallback-to-asm
behavior from `c4cll`.

## Core Rule

The finish line is not just "BIR exists." The finish line is:
- no legacy `ir.*` route
- no backend-to-LLVM fallback route
- no LLVM IR-to-asm rescue route in `c4cll`

## Read First

- [ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md](/workspaces/c4c/ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md)
- [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp)
- [src/codegen/llvm/llvm_codegen.cpp](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
- [src/apps/c4cll.cpp](/workspaces/c4c/src/apps/c4cll.cpp)

## Working Model

Phase order for this runbook:

1. audit every remaining legacy/fallback boundary
2. expand BIR and BIR lowering until emitters can rely on it
3. migrate emitters off `ir.*`
4. delete legacy backend IR conversion/routing
5. delete app-layer LLVM asm rescue

Backend architecture rule for the post-legacy world:

- `BIR` is the one canonical target-neutral backend IR
- target-specific lowering happens after BIR, not inside BIR
- each production backend may introduce its own `MIR` (Machine IR) layer after
  BIR to model ABI/calling-convention/register-constraint details
- liveness, regalloc, stack layout, and machine-level legalization should
  ultimately live on backend-specific `MIR`, not on raw `LIR`
- target printers/debug dumps may exist for `MIR`, while final asm emission is
  produced from the target `MIR` layer

## Non-Goals

- no semantic IR redesign beyond what is required for parity with `ir.*`
- no silent reduction in test coverage to make fallback removal easier
- no leaving LLVM rescue in place under a renamed path

## Step 1. Inventory All Legacy and LLVM Rescue Paths

Goal: produce the exact list of files, call sites, and test surfaces that still
depend on legacy backend IR or LLVM rescue.

Actions:

- inventory `ir.*` consumers
- inventory `lir_to_backend_ir.cpp` and `bir_to_backend_ir.cpp` dependencies
- inventory `emit_legacy()` and legacy routing in backend/codegen
- inventory `c4cll` LLVM IR -> asm rescue and AArch64 fallback normalization
- record which test surfaces currently rely on those paths

Completion Check:

- a concrete removal checklist exists with code references and owner subsystems

## Step 2. Expand BIR Coverage to Parity

Goal: make BIR capable of representing every emitter-facing feature currently
handled by `ir.*`.

Actions:

- fill BIR type-system gaps
- fill instruction and module-surface gaps
- port validation/printer support in lockstep
- port lowering clusters from `lir_to_backend_ir.cpp` into `lir_to_bir.cpp`
- use backend subset coverage such as
  `ctest --test-dir build -L backend --output-on-failure` as the required gate
  for Step 2 slices; do not require full-suite regression during this step


Completion Check:

- BIR can represent the legacy emitter-facing surface without conversion back to
  `ir.*`

## Step 3. Migrate Emitters to BIR

Goal: make x86 and AArch64 emitters consume BIR directly.

Actions:

- update emitter headers and implementation boundaries to use `bir.hpp`
- remove any remaining emitter-side dependence on `BackendModule(ir.*)`
- keep BIR as the target-neutral handoff point even when a backend lowers from
  BIR into target-specific `MIR` before final asm emission
- keep backend-focused validation green as each emitter slice is migrated
- use backend subset coverage such as
  `ctest --test-dir build -L backend --output-on-failure` as the required gate
  for Step 3 slices; do not require full-suite regression during this step

Completion Check:

- emitters include and consume only BIR-owned structures at the backend
  boundary, with any target-specific `MIR` living strictly downstream of that
  boundary

## Step 4. Remove Legacy Backend IR Routing

Goal: delete the old backend IR path once BIR ownership is real.

Actions:

- delete `ir.*`
- delete `lir_to_backend_ir.*`
- delete `bir_to_backend_ir.*`
- remove legacy routing from `backend.cpp`
- validate Step 4 deletions with the backend subset only, using commands such as
  `ctest --test-dir build -L backend --output-on-failure`
- defer full-suite regression until Step 7 unless the work has already reached
  that final validation stage

Completion Check:

- backend has one lowering route and no legacy backend IR files remain

## Step 5. Remove LLVM Rescue From c4cll

Goal: make unsupported backend codegen a hard backend error instead of a silent
LLVM fallback.

Actions:

- remove `lower_llvm_ir_to_asm(...)`
- remove `looks_like_llvm_ir(...)`
- remove `infer_asm_fallback_reasons(...)`
- remove `print_asm_fallback_hint(...)`
- remove `normalize_aarch64_fallback_asm(...)`
- tighten `--codegen asm` so unsupported backend surfaces fail explicitly
- validate Step 5 slices with the backend subset only, using commands such as
  `ctest --test-dir build -L backend --output-on-failure`
- reserve full-suite regression for Step 7 after the backend-only rescue-removal
  work is complete

Completion Check:

- production asm output no longer depends on LLVM rescue

## Step 6. Final Validation

Goal: prove backend behavior is green without legacy backend IR or LLVM rescue
paths before running the full suite.

Actions:

- use only backend-focused validation such as
  `ctest --test-dir build -L backend --output-on-failure` as the default
  regression loop during plan execution to keep iteration fast
- treat the backend subset as the required validation gate for intermediate
  slices; do not require full-suite regression during routine Step 6 work
- run the relevant backend validation and confirm the active migration slice does
  not rely on fallback anymore
- confirm no production code path still routes through LLVM fallback

Completion Check:

- backend-focused validation passes and the removal is complete for the active
  slice

## Step 7. Full Regression

Goal: run the slower full-suite regression only after the backend-focused plan
work is complete.

Actions:

- run the complete regression suite only in this final step, after the
  backend-focused Step 6 validation is already green
- run the complete regression suite
- compare the result against the backend-focused validation used during
  implementation
- record any remaining non-backend fallout as separate follow-on work if needed

Completion Check:

- full regression passes, or any residual failures are explicitly documented and
  split out rather than silently absorbed
