# AArch64 Local Helper Duplication Tail Cleanup Runbook

Status: Active
Source Idea: ideas/open/83_aarch64_local_helper_duplication_tail_cleanup.md

## Purpose

Finish the AArch64-local helper duplication tail left after idea 74 by folding
same-shaped helper logic in globals, atomics, cast, and memory owners into
existing local utilities or narrow owner-local helpers.

## Goal

Reduce repeated AArch64 target-local consumer helper shapes while preserving
owner-specific lowering decisions, diagnostics, and emitted machine records.

## Core Rule

Keep target-local register spelling, scratch selection, prepared-record
consumption, and owner-specific lowering decisions in AArch64-local code; do
not create new shared BIR or prealloc authority for this cleanup.

## Read First

- `ideas/open/83_aarch64_local_helper_duplication_tail_cleanup.md`
- `src/backend/mir/aarch64/codegen/globals.cpp`
- `src/backend/mir/aarch64/codegen/globals.hpp`
- `src/backend/mir/aarch64/codegen/atomics.cpp`
- `src/backend/mir/aarch64/codegen/atomics.hpp`
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `src/backend/mir/aarch64/codegen/cast_ops.hpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `src/backend/mir/aarch64/codegen/operands.cpp`
- `src/backend/mir/aarch64/codegen/operands.hpp`
- `src/backend/mir/aarch64/codegen/prepared_value_home_materialization.cpp`
- `src/backend/mir/aarch64/codegen/prepared_value_home_materialization.hpp`
- `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/fp_value_materialization.hpp`

## Current Targets

- Repeated register-view helper shapes.
- Prepared value ID and prepared-record lookup helpers.
- Storage-plan and frame-slot lookup helpers.
- Diagnostic and prepared-record error wrapping helpers.
- Occupied-register and scratch-selection helper shapes.
- Existing AArch64-local utility owners that already own the abstraction.

## Non-Goals

- Do not move target-local register spelling, scratch selection, or storage
  policy into shared BIR or prealloc code.
- Do not rewrite scalar cast, atomic, global, or memory semantics.
- Do not combine this route with calls, dispatch, printer, or broad backend
  cleanup.
- Do not weaken diagnostics, unsupported-path messages, or test contracts to
  make helper sharing easier.
- Do not introduce named-case shortcuts or testcase-shaped matching.

## Working Model

- `globals.*`, `atomics.*`, `cast_ops.*`, and `memory.*` own their lowering
  decisions and record emission.
- `operands.*`, `prepared_value_home_materialization.*`, and
  `fp_value_materialization.*` may own shared AArch64-local utilities only when
  they already represent the helper shape.
- Owner-local helpers are preferred when a shared utility would blur ownership
  or require new target-neutral authority.
- Each owner slice should leave nearby same-feature paths examined enough to
  prove the change is semantic cleanup rather than narrow testcase repair.

## Execution Rules

- Start with an inventory before implementation and record the selected first
  packet in `todo.md`.
- Keep implementation packets owner-focused unless the exact same helper shape
  is already owned by an existing local utility.
- Preserve diagnostics and unsupported-path behavior unless the supervisor
  explicitly accepts a reviewed diagnostic change.
- Run build proof after each code-changing packet.
- Add or run focused backend tests for the touched owner family before marking
  a step complete.
- Use regression guard before closure if helpers are shared across more than
  one owner.

## Step 1: Inventory Local Helper Duplication

Goal: produce a precise helper map before editing implementation files.

Primary targets:

- `src/backend/mir/aarch64/codegen/globals.*`
- `src/backend/mir/aarch64/codegen/atomics.*`
- `src/backend/mir/aarch64/codegen/cast_ops.*`
- `src/backend/mir/aarch64/codegen/memory.*`
- Existing local utility owners listed in Read First.

Actions:

- List repeated helper shapes for register views, prepared value IDs, storage
  plans, frame slots, diagnostics, prepared-record wrapping, occupied
  registers, and scratch selection.
- Classify each candidate as keep owner-local, fold into an existing
  AArch64-local utility, or narrow into an owner-local helper.
- Identify no-touch candidates that would require new BIR/prealloc authority or
  broad semantic rewriting.
- Select the first owner-focused implementation packet and its proof command.

Completion check:

- `todo.md` records the selected first packet, exact files to touch, proof
  command, and any out-of-scope helper candidates discovered during inventory.

## Step 2: Fold Globals Helper Duplication

Goal: reduce repeated helper shapes in global lowering without changing global
record semantics.

Primary targets:

- `src/backend/mir/aarch64/codegen/globals.cpp`
- `src/backend/mir/aarch64/codegen/globals.hpp`
- Existing local utility owners only when they already own the helper shape.

Actions:

- Fold duplicated register-view, prepared lookup, diagnostics, or scratch
  helper logic into existing AArch64-local utilities when ownership is clear.
- Otherwise narrow repeated global-only logic into local helpers inside the
  globals owner.
- Preserve global-specific lowering decisions and prepared-record diagnostics.

Completion check:

- Build proof passes.
- Focused backend tests or dumps cover representative AArch64 global lowering
  and touched diagnostic paths.

## Step 3: Fold Atomics Helper Duplication

Goal: reduce repeated helper shapes in atomic lowering while preserving
addressing, ordering, and machine-record behavior.

Primary targets:

- `src/backend/mir/aarch64/codegen/atomics.cpp`
- `src/backend/mir/aarch64/codegen/atomics.hpp`
- Existing local utility owners only when they already own the helper shape.

Actions:

- Inspect atomic helper overlap for register views, prepared value lookup,
  occupied-register handling, scratch selection, and diagnostics.
- Fold only same-shaped target-local helper logic whose ownership remains
  obvious after the change.
- Keep atomic-specific record construction and unsupported-path handling inside
  the atomic owner.

Completion check:

- Build proof passes.
- Focused backend tests cover representative AArch64 atomic lowering and any
  changed error paths.

## Step 4: Fold Cast Helper Duplication

Goal: reduce repeated helper shapes in scalar and floating-point cast lowering
without changing cast semantics.

Primary targets:

- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `src/backend/mir/aarch64/codegen/cast_ops.hpp`
- `src/backend/mir/aarch64/codegen/fp_value_materialization.*` only when it
  already owns the helper shape.

Actions:

- Inspect cast helper overlap for register views, prepared value lookup,
  floating-point materialization, diagnostics, and scratch handling.
- Fold repeated cast helper logic into existing local utilities or narrower
  cast-local helpers.
- Preserve scalar cast, FP cast, and unsupported-path behavior exactly unless a
  reviewed semantic fix is separately approved.

Completion check:

- Build proof passes.
- Focused backend tests cover representative AArch64 scalar and FP cast
  lowering plus touched diagnostics.

## Step 5: Fold Memory Helper Duplication

Goal: reduce repeated helper shapes in memory lowering while preserving storage
plan, frame-slot, addressing, and record behavior.

Primary targets:

- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `src/backend/mir/aarch64/codegen/prepared_value_home_materialization.*` only
  when it already owns the helper shape.

Actions:

- Inspect memory helper overlap for prepared IDs, storage-plan lookup,
  frame-slot lookup, diagnostics, register views, occupied registers, and
  scratch selection.
- Fold same-shaped helper logic into existing local utilities where that
  reduces duplication without relocating memory lowering policy.
- Keep addressing legality, storage-plan interpretation, and record emission in
  the memory owner.

Completion check:

- Build proof passes.
- Focused backend tests cover representative AArch64 memory lowering,
  frame-slot/storage-plan paths, and touched diagnostics.

## Step 6: Closure Validation And Review Readiness

Goal: prove helper contraction did not weaken diagnostics or relocate target
authority outside the source idea.

Actions:

- Run the supervisor-selected broader backend validation set after owner
  packets are complete.
- Run regression guard if any helper was shared across more than one owner.
- Check the final diff against source-idea reject signals: shared authority
  without target-neutral proof, testcase-shaped matches, diagnostic weakening,
  and multi-owner rewrites without intermediate proof.
- Record any residual helper family that should become a separate source idea
  instead of being folded into this runbook.

Completion check:

- Canonical proof logs are ready for supervisor review.
- `todo.md` identifies residual candidates, if any, as follow-up idea material
  rather than hidden active-plan scope.
