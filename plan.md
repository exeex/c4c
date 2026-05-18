# AArch64 Stack Frame SP Alignment

Status: Active
Source Idea: ideas/open/288_aarch64_stack_frame_sp_alignment.md
Activated From: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md

## Purpose

Repair the AArch64 backend frame-size and stack-slot layout path that can emit
function stack adjustments which leave `sp` misaligned.

## Goal

Make generated AArch64 functions preserve 16-byte stack-pointer alignment
while keeping local scalar, pointer, and array stack slots addressable.

## Core Rule

Fix semantic frame layout and stack adjustment. Do not improve this route by
changing tests, expected outputs, runner behavior, CTest registration,
allowlists, unsupported classifications, timeout policy, parser, sema, or
c-testsuite filename/frame-size shortcuts.

## Read First

- `ideas/open/288_aarch64_stack_frame_sp_alignment.md`
- `todo.md`
- `tests/c/external/c-testsuite/src/00004.c`
- `tests/c/external/c-testsuite/src/00005.c`
- `tests/c/external/c-testsuite/src/00013.c`
- `tests/c/external/c-testsuite/src/00014.c`
- `tests/c/external/c-testsuite/src/00015.c`
- `tests/c/external/c-testsuite/src/00016.c`

## Current Targets

- AArch64 backend frame-size computation.
- AArch64 prologue/epilogue stack adjustment.
- Local stack-slot offsets when frame padding is required.
- Minimal bus-error proof case: `src/00004.c`.
- Nearby local pointer/array bus-error proof cases: `src/00005.c`,
  `src/00013.c`, `src/00014.c`, `src/00015.c`, and `src/00016.c`.

## Non-Goals

- Do not change runner behavior, allowlists, expectations, unsupported
  classifications, timeout settings, or CTest registration.
- Do not change parser or sema behavior.
- Do not add named-case shortcuts, exact frame-size shortcuts, local-variable
  spelling shortcuts, or emitted-instruction-text matching.
- Do not take on broad pointer semantics, array semantics, struct layout,
  aggregate ABI, function-pointer semantics, broad variadic ABI, libc
  behavior, or timeout-sensitive cases unless proven to be directly downstream
  of the same SP/frame-alignment defect.
- Do not fold compare/branch printer/lowering work into this route.

## Working Model

The refreshed inventory found a 28-case `Bus error` cluster. The smallest
representative, `src/00004.c`, emits a 24-byte stack frame, which violates the
AArch64 requirement that `sp` remain 16-byte aligned. The first repair should
round or pad frame allocation semantically, then verify local slot addressing
still uses the correct offsets.

## Execution Rules

- Keep packets small and prove each packet with a fresh build or compile/run
  command chosen by the supervisor.
- Inspect generated AArch64 assembly when debugging, but accept progress only
  through semantic behavior and relevant test proof.
- Preserve local-slot load/store correctness when adding alignment padding.
- Document unrelated remaining bus-error or pointer/array blockers in
  `todo.md` instead of broadening this route.
- Escalate to reviewer scrutiny if the implementation appears to special-case
  one c-testsuite filename, one frame size, or one local-variable layout.

## Steps

### Step 1: Establish Minimal SP Alignment Failure

Goal: Prove and localize the smallest bus-error failure to misaligned stack
frame allocation.

Primary target: `tests/c/external/c-testsuite/src/00004.c`.

Actions:

- Reproduce the current AArch64 backend behavior for `src/00004.c` with the
  supervisor-selected narrow command.
- Inspect the generated function prologue, epilogue, frame size, and local
  stack-slot offsets.
- Confirm where the frame size should become a multiple of 16 and how local
  offsets should remain valid after padding.
- Record the observed failure shape and owned backend surfaces in `todo.md`.

Completion check: `todo.md` names the current failure, the frame/layout
surface to change, and the proof command/log used for the baseline.

### Step 2: Repair General AArch64 Frame Alignment

Goal: Ensure generated AArch64 frames preserve 16-byte `sp` alignment without
breaking local stack-slot addressing.

Primary target: AArch64 backend frame sizing and prologue/epilogue emission.

Actions:

- Add or repair the semantic path that rounds/pads stack frame size to the
  AArch64 alignment requirement.
- Keep local slot offsets, frame-base assumptions, and stack restores
  consistent with the aligned frame size.
- Ensure call boundaries see an aligned `sp` when generated functions contain
  calls.
- Build and run the supervisor-selected narrow proof for `src/00004.c`.

Completion check: `src/00004.c` no longer fails from SP misalignment, and
generated frames keep `sp` 16-byte aligned.

### Step 3: Expand to Nearby Pointer and Array Locals

Goal: Show the frame-alignment repair generalizes beyond the minimal local
pointer case.

Primary target: `src/00005.c`, `src/00013.c`, `src/00014.c`, `src/00015.c`,
and `src/00016.c`.

Actions:

- Run the supervisor-selected subset covering the nearby bus-error local
  pointer/array representatives.
- Inspect any remaining failures and separate SP/frame-alignment defects from
  broader pointer, array, struct, function-pointer, or local-state owners.
- Repair only defects that are inside this idea's frame-size, stack-adjustment,
  and local-slot offset scope.
- Record proof results and any deferred blockers in `todo.md`.

Completion check: the nearby local pointer/array family passes for this owner,
or `todo.md` clearly separates remaining failures that belong to another
focused idea.

### Step 4: Sample the Wider Bus-Error Cluster and Decide Closure

Goal: Confirm the route is not overfit to the first six cases and decide
whether the source idea is complete.

Primary target: remaining `Bus error` cases from the refreshed inventory.

Actions:

- Ask the supervisor for a broader related-case subset after the nearby proof
  is green.
- Include representative struct, function-pointer, and attribute/function
  pointer bus-error cases only to classify whether SP/frame alignment remains
  the owner.
- Keep timeout-sensitive cases and compare/branch compile-stage gaps out of
  this acceptance route.
- If the broader subset exposes a separate semantic owner, record a follow-on
  idea instead of absorbing it into this route.

Completion check: broader bus-error proof supports closure of this focused
idea, or `todo.md` records the exact remaining source-idea gap.
