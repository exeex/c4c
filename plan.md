# AArch64 Extern Global Array Addressing Runbook

Status: Active
Source Idea: ideas/open/11_backend_aarch64_extern_global_array_addressing_plan.md
Activated from: ideas/open/11_backend_aarch64_extern_global_array_addressing_plan.md

## Purpose

Turn the next AArch64 global-addressing follow-on into one explicit backend-owned extern-array slice without reopening broad pointer arithmetic or mixed local/global lowering.

## Goal

Promote one bounded extern global array element-load case onto the AArch64 asm path with targeted backend tests and one matching runtime validation.

## Core Rule

Keep the slice narrower than general pointer arithmetic. Stop and split the work if implementation starts pulling in broad GEP generalization, pointer-difference lowering, or mixed string/global address handling in one patch.

## Read First

- `ideas/open/11_backend_aarch64_extern_global_array_addressing_plan.md`
- `tests/backend/backend_lir_adapter_tests.cpp`
- runtime cases under `tests/c/internal/backend_case/`

## Current Targets

- synthetic backend coverage for one extern global array base-address plus indexed element-load seam
- one bounded runtime case promoted through `BACKEND_OUTPUT_KIND=asm` if an exact frontend-emitted candidate exists
- backend-owned asm that assembles and preserves the expected result

## Non-Goals

- general aggregate pointer arithmetic
- mixed local/global address lowering in one patch
- pointer-difference and round-trip work such as `global_char_pointer_diff`, `global_int_pointer_diff`, or `global_int_pointer_roundtrip`
- broad relocation-model cleanup

## Working Model

- start from the smallest real frontend-emitted candidate that matches the synthetic extern-array seam
- make the backend seam explicit before promoting runtime coverage
- prefer one synthetic backend contract and one runtime case over broadening the emitter surface

## Execution Rules

- add or tighten tests before implementation
- use Clang/LLVM output as the reference when the exact lowering shape is unclear
- if no bounded runtime case exists yet, keep the runtime promotion deferred and record that explicitly rather than widening scope
- record any adjacent pointer/address follow-on work back into `ideas/open/` rather than absorbing it here

## Step 1: Confirm The Exact Extern-Array Slice

Goal: identify the narrowest frontend-emitted case that matches the synthetic extern global array seam.

Primary target: `make_extern_global_array_load_module()` and any matching runtime candidate.

Actions:

- inspect the existing synthetic backend fixture and runtime candidates for the smallest extern-array decay plus indexed-load seam
- capture the relevant AArch64 LIR or LLVM IR shape for the chosen case
- confirm whether a bounded runtime case exists without dragging in broad pointer generalization

Completion check:

- one exact synthetic target is selected
- the required extern-array address-formation operations are written down in concrete backend terms
- runtime promotion is either justified by a bounded case or explicitly deferred

## Step 2: Lock The Backend Test Contract

Goal: define targeted tests for the extern-array seam before implementation.

Primary target: `tests/backend/backend_lir_adapter_tests.cpp`

Actions:

- tighten synthetic tests so the extern-array slice stops accepting LLVM IR fallback
- cover the exact base-address formation and indexed element-load seam needed by the chosen case
- avoid introducing generalized pointer-arithmetic expectations

Completion check:

- tests fail for the missing seam and describe the intended lowering
- the test surface names only the bounded extern-array operations needed for this slice

## Step 3: Implement The Narrow Backend-Owned Lowering

Goal: make the AArch64 backend lower the selected extern-array address-formation seam and matching indexed load.

Primary target: `src/backend/aarch64/codegen/emit.cpp`

Actions:

- implement the minimal representation needed for extern-array base-address formation
- wire the first indexed element-load lowering through the backend-owned path
- keep unrelated local-memory and pointer-generalization logic out of scope

Completion check:

- targeted backend tests pass
- the lowering path is explicit rather than falling back to LLVM IR

## Step 4: Promote One Runtime Case If Bounded

Goal: validate the new seam end to end with one runtime case, but only if a bounded frontend-emitted case exists.

Primary target: one case under `tests/c/internal/backend_case/`

Actions:

- route the selected runtime case through `BACKEND_OUTPUT_KIND=asm` if the case matches the narrow extern-array seam
- otherwise record why runtime promotion remains deferred for this slice
- verify the asm output assembles and preserves the expected result when promoted

Completion check:

- the chosen runtime test passes through the asm path, or the lack of a bounded runtime candidate is explicitly recorded
- no extra runtime cases are promoted as part of this slice

## Step 5: Regressions And Handoff

Goal: prove the slice is monotonic and leave the next follow-on obvious.

Actions:

- run targeted backend tests and any promoted runtime case
- run the broader regression checks from the active execution prompt
- record deferred follow-on cases if broader pointer/address work remains

Completion check:

- regression results are monotonic
- `plan_todo.md` reflects completed work, deferred work, and the next intended slice
