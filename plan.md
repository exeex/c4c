# AArch64 `atomics.md` Shard Redistribution Runbook

Status: Active
Source Idea: ideas/open/266_aarch64_atomics_markdown_shard_implementation_redistribution.md

## Purpose

Create a real AArch64 atomics owner for the current compiled backend route,
then remove the stale markdown shard.

## Goal

Reconcile `src/backend/mir/aarch64/codegen/atomics.md` into
`atomics.cpp` and `atomics.hpp` without inventing missing prepared atomic
facts or broadening AArch64 backend behavior.

## Core Rule

This is an ownership and fail-closed boundary route. Preserve supported
behavior, keep unsupported atomic forms explicit, and do not port the legacy
exclusive-loop reference surface without current prepared facts.

## Read First

- `ideas/open/266_aarch64_atomics_markdown_shard_implementation_redistribution.md`
- `src/backend/mir/aarch64/codegen/atomics.md`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- focused AArch64 backend tests that cover atomic lowering, diagnostics, or
  explicit unsupported behavior

## Current Targets

- New `src/backend/mir/aarch64/codegen/atomics.cpp`.
- New `src/backend/mir/aarch64/codegen/atomics.hpp`.
- Current accepted atomic-family construction, lowering, routing, or
  diagnostic hooks if prepared facts already support them.
- Deletion of `src/backend/mir/aarch64/codegen/atomics.md` after durable valid
  content has been reconciled.

## Non-Goals

- Do not invent missing prepared atomic facts.
- Do not implement all AArch64 exclusive-loop lowering from the reference
  shard in one broad jump.
- Do not change unrelated memory, register allocation, frame, ABI, or printer
  behavior.
- Do not rewrite tests or expectations to make unsupported atomics look
  supported.
- Do not redistribute neighboring AArch64 markdown shards.

## Working Model

`atomics.cpp` and `atomics.hpp` should become the live compiled boundary for
atomic-family AArch64 codegen. The markdown shard is a reconciliation source,
not an implementation recipe: keep only facts that match the current prepared
MIR/backend model, route dispatch through the atomics owner where appropriate,
and leave unsupported forms explicit or fail-closed.

## Execution Rules

- Prefer small behavior-preserving ownership moves over broad semantic
  rebuilds.
- Treat legacy fixed-register exclusive-loop details as reference material
  unless the current route exposes the needed prepared facts and scratch
  policy.
- Keep unsupported atomic forms visible as explicit errors, diagnostics, or
  deferred handling; do not silently lower them through generic placeholders.
- Do not add testcase-shaped shortcuts or named-case-only atomic lowering.
- Do not weaken, delete, or reclassify tests to claim atomic support.
- If implementation crosses broad codegen owners, prove the affected backend
  route with a focused build plus backend tests.

## Ordered Steps

### Step 1: Audit The Atomic Shard And Current Route

Goal: Identify which facts in `atomics.md` can safely map to the current
compiled AArch64 backend route.

Primary target: `src/backend/mir/aarch64/codegen/atomics.md`

Actions:

- Inspect the markdown shard and classify each durable section as current,
  unsupported/deferred, or obsolete reference-only material.
- Search current AArch64 codegen owners for atomic-family routing, lowering,
  diagnostics, or explicit unsupported handling.
- Identify whether any accepted current atomic facts already belong in a new
  atomics owner.
- Record the audit result in `todo.md` before creating or moving code.

Completion check:

- `todo.md` names the current facts to keep, the unsupported facts to preserve
  as explicit fail-closed behavior, and any concrete routing or owner issue to
  fix.

### Step 2: Create The Atomics Owner Boundary

Goal: Add `atomics.cpp` and `atomics.hpp` as the compiled owner for
atomic-family AArch64 codegen.

Primary target: `src/backend/mir/aarch64/codegen/atomics.cpp`

Actions:

- Create the atomics source and header using nearby AArch64 codegen owner
  conventions.
- Move or add only current accepted atomic construction, lowering, routing, or
  diagnostic hooks that have prepared backend facts.
- Keep unsupported atomic forms explicit and fail-closed.
- Route broad dispatch through the atomics owner instead of embedding
  atomic-family bodies in `dispatch.cpp`, `instruction.cpp`, or `memory.cpp`.

Completion check:

- `atomics.cpp` and `atomics.hpp` build, own the current atomic-family
  boundary, and do not claim unsupported full atomic lowering.

### Step 3: Delete The Stale Markdown Shard

Goal: Remove `atomics.md` after all durable valid content has either been
absorbed into compiled owners or explicitly rejected as unsupported/deferred.

Primary target: `src/backend/mir/aarch64/codegen/atomics.md`

Actions:

- Delete the markdown shard.
- Confirm no source list, docs index, classification index, or build/test
  reference still expects the deleted markdown file.
- Keep neighboring AArch64 markdown shards untouched.

Completion check:

- `atomics.md` is gone, and all repository references either point to the new
  compiled owner or no longer mention the stale shard.

### Step 4: Prove Focused Backend Behavior

Goal: Show that atomics ownership redistribution preserves supported behavior
and keeps unsupported behavior explicit.

Primary target: focused AArch64 backend proof.

Actions:

- Run the narrow proof command chosen by the supervisor for the implementation
  packet.
- Include focused backend tests that exercise the affected AArch64 codegen
  route, atomic diagnostics, or explicit unsupported handling when available.
- Escalate to broader backend validation if routing changes affect shared
  dispatch, instruction, memory, or printer behavior.

Completion check:

- Fresh proof is recorded in `test_after.log` by the executor, and `todo.md`
  summarizes the preserved supported behavior plus explicit unsupported
  behavior evidence.

## Completion Criteria

- `atomics.cpp` and `atomics.hpp` exist and own the current atomic-family
  boundary.
- `src/backend/mir/aarch64/codegen/atomics.md` is deleted.
- Broad owners do not contain new atomic-family bulk implementation.
- Supported behavior is preserved, and unsupported behavior remains explicit.
- Focused backend proof covers the affected AArch64 codegen route.
