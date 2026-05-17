# AArch64 `intrinsics.md` Shard Redistribution Runbook

Status: Active
Source Idea: ideas/open/268_aarch64_intrinsics_markdown_shard_implementation_redistribution.md

## Purpose

Create a real AArch64 intrinsics owner for the current compiled backend route,
then remove the stale markdown shard.

## Goal

Reconcile `src/backend/mir/aarch64/codegen/intrinsics.md` into
`intrinsics.cpp` and `intrinsics.hpp` without turning the legacy reference
surface into an unbounded platform-intrinsic feature port.

## Core Rule

This is an ownership and explicit-boundary route. Preserve current accepted
target-intrinsic lowering, keep unsupported or deferred families fail-closed or
visible, and do not port historical NEON, CRC, cache, hint, frame-address, or
soft-float helper behavior unless current prepared facts and tests already
authorize it.

## Read First

- `ideas/open/268_aarch64_intrinsics_markdown_shard_implementation_redistribution.md`
- `src/backend/mir/aarch64/codegen/intrinsics.md`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`
- `src/backend/mir/aarch64/codegen/float_ops.cpp`
- `src/backend/mir/aarch64/codegen/float_ops.hpp`
- `src/backend/mir/aarch64/BACKEND_CASE_BRINGUP_MATRIX.md`
- focused backend tests that cover prepared intrinsic carriers, selected
  AArch64 intrinsic nodes, machine-printer spelling, diagnostics, or explicit
  unsupported behavior

## Current Targets

- New `src/backend/mir/aarch64/codegen/intrinsics.cpp`.
- New `src/backend/mir/aarch64/codegen/intrinsics.hpp`.
- Current accepted intrinsic dispatch, record construction, selection-status,
  printer validation, final instruction spelling, diagnostics, or explicit
  deferred hooks if prepared intrinsic facts already support them.
- Deletion of `src/backend/mir/aarch64/codegen/intrinsics.md` after durable
  valid content has been reconciled.

## Non-Goals

- Do not port the full legacy NEON, CRC, cache, hint, frame-address,
  return-address, thread-pointer, or binary128 soft-float helper surface in
  this ownership cleanup.
- Do not invent missing prepared intrinsic carriers or feature-gating facts.
- Do not change scalar float, F128, memory, call, return, allocation, or
  printer behavior outside an intrinsic ownership need.
- Do not add named-intrinsic shortcuts just to make a narrow testcase pass.
- Do not weaken, delete, or reclassify tests to overstate intrinsic support.
- Do not redistribute neighboring AArch64 markdown shards.

## Working Model

`intrinsics.cpp` and `intrinsics.hpp` should become the live compiled boundary
for AArch64 intrinsic codegen. The markdown shard is reconciliation input, not
an implementation recipe: keep only behavior that matches the current prepared
carrier and machine-node route, move broad-owner intrinsic logic behind the
intrinsics owner where appropriate, and leave unsupported families explicit or
fail-closed.

## Execution Rules

- Prefer small behavior-preserving ownership moves over broad semantic
  rebuilds.
- Treat legacy platform-intrinsic sections as reference material unless the
  current route already exposes structured prepared carrier facts, homes,
  features, operand roles, vector shape, signedness, and memory-access
  metadata.
- Keep unsupported intrinsic families visible as explicit errors, diagnostics,
  or deferred handling; do not silently emit placeholder instructions.
- Keep selected intrinsic-machine records semantically checked against their
  prepared provenance.
- Do not move unrelated float, memory, call, return, or printer code just
  because the old markdown shard mentioned adjacent helper behavior.
- If implementation crosses broad codegen owners, prove the affected backend
  route with a focused build plus backend tests.

## Ordered Steps

### Step 1: Audit The Intrinsics Shard And Current Route

Goal: Identify which facts in `intrinsics.md` can safely map to the current
compiled AArch64 backend route.

Primary target: `src/backend/mir/aarch64/codegen/intrinsics.md`

Actions:

- Inspect the markdown shard and classify each durable section as current,
  unsupported/deferred, or obsolete reference-only material.
- Search current AArch64 codegen owners for intrinsic routing, prepared
  carrier lookup, record construction, selection status, printer spelling,
  diagnostics, or explicit unsupported handling.
- Identify whether current accepted intrinsic facts already belong in a new
  intrinsics owner.
- Record the audit result in `todo.md` before creating or moving code.

Completion check:

- `todo.md` names the current intrinsic facts to keep, the unsupported or
  deferred families to preserve as explicit fail-closed behavior, and any
  concrete routing or owner issue to fix.

### Step 2: Create The Intrinsics Owner Boundary

Goal: Add `intrinsics.cpp` and `intrinsics.hpp` as the compiled owner for
AArch64 intrinsic codegen.

Primary target: `src/backend/mir/aarch64/codegen/intrinsics.cpp`

Actions:

- Create the intrinsics source and header using nearby AArch64 codegen owner
  conventions.
- Move or add only current accepted intrinsic routing, record-construction,
  validation, printing, diagnostic, or deferred-boundary hooks that have
  prepared backend facts.
- Keep unsupported intrinsic families explicit and fail-closed.
- Route broad dispatch, instruction, or printer paths through the intrinsics
  owner instead of embedding intrinsic bodies in `dispatch.cpp`,
  `instruction.cpp`, `machine_printer.cpp`, `float_ops.cpp`, or unrelated
  owners.

Completion check:

- `intrinsics.cpp` and `intrinsics.hpp` build, own the current intrinsic
  boundary, and do not claim unsupported platform-intrinsic support.

### Step 3: Delete The Stale Markdown Shard

Goal: Remove `intrinsics.md` after all durable valid content has either been
absorbed into compiled owners or explicitly rejected as unsupported/deferred.

Primary target: `src/backend/mir/aarch64/codegen/intrinsics.md`

Actions:

- Delete the markdown shard.
- Confirm no source list, docs index, classification index, bring-up matrix,
  or build/test reference still expects the deleted markdown file.
- Keep neighboring AArch64 markdown shards untouched.

Completion check:

- `intrinsics.md` is gone, and all repository references either point to the
  new compiled owner or no longer mention the stale shard.

### Step 4: Prove Focused Backend Behavior

Goal: Show that intrinsic ownership redistribution preserves supported
behavior and keeps unsupported behavior explicit.

Primary target: focused AArch64 backend proof.

Actions:

- Run the narrow proof command chosen by the supervisor for the implementation
  packet.
- Include focused backend tests that exercise affected AArch64 prepared
  intrinsic carriers, selected machine-node routing, instruction validation,
  machine-printer spelling, diagnostics, or explicit unsupported behavior when
  available.
- Escalate to broader backend validation if routing changes affect shared
  dispatch, instruction, printer, or assembler behavior.

Completion check:

- Fresh proof is recorded in `test_after.log` by the executor, and `todo.md`
  summarizes the preserved supported behavior plus explicit unsupported
  behavior evidence.

## Completion Criteria

- `intrinsics.cpp` and `intrinsics.hpp` exist and own the current intrinsic
  boundary or explicit deferred handling.
- `src/backend/mir/aarch64/codegen/intrinsics.md` is deleted.
- Broad owners do not contain new intrinsic bulk implementation.
- Supported behavior is preserved, and unsupported behavior remains explicit.
- Focused backend proof covers the affected AArch64 codegen route.
