# BIR Cutover And Legacy Cleanup Runbook

Status: Active
Source Idea: ideas/open/06_bir_cutover_and_legacy_cleanup.md
Activated from: prompts/ACTIVATE_PLAN.md

## Purpose

Complete the backend migration by making BIR the default path, deleting
legacy-only backend plumbing, and leaving one clear backend test story.

## Goal

Ship a BIR-default backend flow with only a short-lived escape hatch if needed,
while materially reducing legacy routing, compatibility code, and legacy-only
tests.

## Core Rule

Do not widen this effort into unrelated backend cleanup. Keep each slice narrow,
test-backed, and directly tied to the cutover from the legacy backend path to
the BIR-first path.

## Read First

- [`ideas/open/06_bir_cutover_and_legacy_cleanup.md`](/workspaces/c4c/ideas/open/06_bir_cutover_and_legacy_cleanup.md)
- [`prompts/EXECUTE_PLAN.md`](/workspaces/c4c/prompts/EXECUTE_PLAN.md)

## Scope

- keep BIR as the authoritative backend route for supported slices
- reduce or delete backend-side legacy lowering and compatibility plumbing
- clean up surviving transitional `backend_ir` naming where practical
- make backend tests primarily validate the BIR-oriented layered structure

## Non-Goals

- inventing another backend IR abstraction after BIR
- broad backend cleanup outside the cutover surfaces
- deleting the remaining fallback before unsupported slices are accounted for
- merging adjacent parser/frontend initiatives into this backend plan

## Working Model

1. continue from the highest-priority incomplete item in [`todo.md`](/workspaces/c4c/todo.md)
2. keep each slice backend-focused and small enough for targeted validation
3. preserve fallback behavior only where current unsupported slices still need it
4. record any adjacent but separate work back into `ideas/open/`

## Execution Rules

- start by updating [`todo.md`](/workspaces/c4c/todo.md) with the exact slice for this iteration
- add or update the narrowest backend regression before deleting or reshaping a fallback seam
- prefer removing legacy-only dependencies after proving the surviving structured backend module already carries the needed contract
- run targeted backend validation before broader regression checks
- when a fallback remains necessary, keep the retained surface explicit and minimal

## Ordered Steps

### Step 1: Establish the cutover surface

Goal: identify the backend routing entrypoints, fallback knobs, and validating
tests that control BIR-vs-legacy behavior.

Completion check:

- the active backend route-selection seam is understood
- the relevant backend-focused validation set is named

### Step 2: Flip the default to BIR

Goal: make BIR the authoritative backend route for supported slices.

Completion check:

- default execution goes through BIR for supported slices
- targeted tests cover the BIR-default route
- any retained fallback is explicitly temporary and limited

### Step 3: Remove legacy backend plumbing

Goal: reduce or delete backend-side code that only served the old route.

Primary targets:

- legacy adapter-first lowering entrypoints
- backend parsing helpers used only by the old path
- compatibility wrappers between LIR and the old backend IR layers
- transitional debug and print surfaces tied only to legacy flow

Actions:

- trace each remaining live `legacy_fallback` consumer before deleting it
- remove dead fallback branches once a targeted regression guard exists
- keep unsupported slices working when they still require a legacy text path

Completion check:

- legacy-only backend plumbing is materially reduced
- any remaining fallback is narrow and justified by unsupported slices

### Step 4: Clean up surviving names

Goal: make the surviving backend pipeline read as HIR -> LIR -> BIR -> target
emission.

Primary targets:

- transitional `backend_ir` names
- lowering entrypoints whose names still describe the old model
- comments or docs that still describe the old backend IR as authoritative

Completion check:

- the surviving backend path is described in BIR terms
- legacy transitional naming is removed or isolated behind explicit shims

## Carry-Forward Notes

- The last serious Step 3 candidate was the AArch64
  `needs_nonminimal_lowering && legacy_fallback` early return in
  [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp).
- A direct experiment removed that early return and ran
  `ctest --test-dir build -R backend --output-on-failure`.
- Result: 289/290 backend-regex tests still passed. The only failure was
  `backend_contract_aarch64_extern_global_array_object`.
- That failing case goes through the real `c4cll --codegen asm` production
  route in [`src/codegen/llvm/llvm_codegen.cpp`](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp),
  not just the unit-test seam.
- The broken output lost the expected ELF `.extern ext_arr` contract and fell
  onto the generic AArch64 extern-global addressing path:
  `adrp x8, :got:ext_arr`
  `ldr x8, [x8, :got_lo12:ext_arr]`
  `ldr w0, [x8, #4]`
- The explicit lowered-backend tests for structured extern global arrays already
  expect the surviving fast path to render:
  `.extern ext_arr`
  `adrp x8, ext_arr`
  `add x8, x8, :lo12:ext_arr`
  `ldr w0, [x8, #4]`
- Most likely fix direction:
  preserve the removal of the broad AArch64 early return, but port the
  `extern_global_array` legacy-LIR route onto the structured backend matcher so
  the production route still matches
  `parse_minimal_extern_global_array_load_slice(const BackendModule&)` and
  emits `emit_minimal_extern_global_array_load_asm(...)`.
- Do not paper over this by only changing the contract test. The desired result
  is to keep the backend on the structured AArch64 path without the
  `legacy_fallback` early return.

### Step 5: Finish the backend test migration

Goal: make the BIR-oriented layered test structure the primary backend
validation path for the cutover.

Primary targets:

- `tests/c/internal/backend_*`
- `tests/backend/*`
- backend-regex filtered regression runs used during the cutover

Completion check:

- backend tests primarily validate the BIR-based path
- obsolete transitional-path coverage is reduced
- full-suite validation remains monotonic
