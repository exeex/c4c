# AArch64 Scalar Cast ALU Publication Prepared Authority Repair Runbook

Status: Active
Source Idea: ideas/open/55_aarch64_scalar_cast_alu_publication_prepared_authority_repair.md

## Purpose

Repair duplicate scalar publication authority across AArch64 cast and ALU
publication paths so pointer-derived scalar values consume prepared source and
home facts instead of stale live registers or local producer recovery.

## Goal

Make `cast_ops.cpp` and `alu.cpp` select authoritative prepared sources for
pointer-derived scalar publications, preserving needed sources before
block-entry or edge publication overwrites their physical registers when
necessary.

## Core Rule

Do not fix the `00204` aggregate byte-copy failure through named-case matching,
textual instruction suppression, raw BIR scans, or expectation downgrades.
Every accepted change must move source/home authority into prepared facts or a
narrow shared prepared query.

## Read First

- `ideas/open/55_aarch64_scalar_cast_alu_publication_prepared_authority_repair.md`
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- Existing prepared source/home/publication surfaces before adding new ones:
  - `src/backend/prealloc/prepared_lookups.cpp`
  - `src/backend/prealloc/prepared_lookups.hpp`
  - `src/backend/prealloc/publication_plans.cpp`
  - `src/backend/prealloc/publication_plans.hpp`
  - `src/backend/prealloc/regalloc/consumer_moves.cpp`
  - `src/backend/prealloc/regalloc/consumer_moves.hpp`

## Current Targets

- Primary owned files:
  - `src/backend/mir/aarch64/codegen/cast_ops.cpp`
  - `src/backend/mir/aarch64/codegen/alu.cpp`
- Prepared lookup, publication-plan, or consumer-move files may be touched only
  when a shared source/home query or preservation placement is required for the
  cast plus ALU publication contract.

## Non-Goals

- Do not change AAPCS64 call staging, variadic layout constants, edge-copy
  mechanics, or memory cursor writeback behavior.
- Do not edit `dispatch_value_materialization.cpp` unless a bounded audit proves
  a shared prepared query must be exposed for cast/ALU consumers.
- Do not change arithmetic opcode spelling, target-local scratch ordering, or
  sign/zero-extension semantics except to consume the correct prepared source.
- Do not accept unrelated dirty `memory.cpp` or `dispatch_edge_copies.cpp`
  changes under this idea unless the supervisor explicitly routes a coherent
  combined slice.

## Working Model

- Pointer-derived values such as a selected aggregate pointer and its byte
  offset must keep pointer and integer-offset authority separate.
- Cast lowering must derive source width and source register identity from
  prepared publication/source facts when those facts exist.
- ALU stack-publication paths must not republish a stale register home when a
  prepared value-home, scalar-publication, or source-producer fact identifies
  the full-width source.
- If a later cast or ALU publication needs an original value after publication
  overwrites its register, the repair belongs in prepared preservation or
  placement, not in a mutable-memory reload at the join.

## Execution Rules

- Inspect existing prepared facts before adding a new query.
- Add one narrow shared query only if existing prepared value-home,
  storage-plan, scalar-publication, block-entry, edge-publication, and
  source-producer facts cannot express the needed source for both cast and ALU
  consumers.
- Keep implementation slices small enough to validate with build proof plus a
  focused backend subset selected by the supervisor.
- Keep same-feature probes active; do not rely on one `00204` case as proof.
- Record routine packet progress and proof in `todo.md`, not by rewriting this
  runbook.

## Step 1 - Trace The First Bad Publication

Goal: identify the exact cast or ALU lowering path that emits the stale-source
scalar publication.

Primary target:

- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Reproduce or inspect the focused `00204` failure route enough to find the
  first stale-source scalar publication site.
- Trace the source value, destination home, prepared value-home, scalar
  publication, block-entry, and edge-publication facts visible at that site.
- Distinguish a cast-owned source-selection bug from an ALU-owned
  stack-publication bug and from a missing prepared preservation placement.
- Check nearby same-feature paths so the route does not become tied to
  `myprintf`, `%t45`, `%t49`, or `vaarg.join.39`.

Completion check:

- `todo.md` records the owning lowering path, the prepared facts that should
  control source selection, and whether the next step is cast, ALU, or prepared
  preservation work.

## Step 2 - Consume Existing Prepared Source/Home Authority

Goal: make the identified cast or ALU path use existing prepared facts when
they already carry the authoritative source and destination home.

Primary target:

- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Replace stale live-register assumptions with prepared value-home,
  storage-plan, scalar-publication, block-entry, edge-publication, or
  source-producer facts where available.
- Preserve target-local instruction spelling and scratch-order behavior.
- Avoid raw value-name matching, same-block producer walks, or mutable-memory
  reloads as the durable source of truth.

Completion check:

- The traced stale publication no longer reuses the overwritten pointer
  register as a 32-bit scalar offset source for semantic reasons, and focused
  build plus test proof is recorded in `todo.md`.

## Step 3 - Add A Narrow Shared Query If Needed

Goal: expose only the missing prepared contract required by both cast and ALU
publication consumers.

Primary target:

- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/regalloc/consumer_moves.cpp`
- `src/backend/prealloc/regalloc/consumer_moves.hpp`

Actions:

- Add a prepared scalar cast/publication source query only after proving
  existing prepared facts cannot answer the source/home question.
- Keep the query keyed by semantic source/home identity rather than testcase
  names or local BIR shape.
- Wire both cast and ALU consumers through the same authority when both need
  the fact.

Completion check:

- New or extended prepared query has a narrow documented consumer contract, and
  no new local scans or named-case shortcuts are introduced.

## Step 4 - Preserve Sources Before Publication Overwrite

Goal: place or preserve source values before block-entry or edge publication
overwrites their physical register when no safe current home exists.

Primary target:

- `src/backend/prealloc/regalloc/consumer_moves.cpp`
- `src/backend/prealloc/regalloc/consumer_moves.hpp`
- related publication-plan files only as needed

Actions:

- Prove the source is otherwise unavailable from an immutable prepared home.
- Add source preservation or relocation at the prepared planning layer.
- Keep the preservation rule general for scalar cast and ALU publication
  consumers.

Completion check:

- Later cast/ALU publication can consume a preserved authoritative source
  without reloading mutated local memory or relying on a stale live register.

## Step 5 - Validate Focused And Broader Backend Behavior

Goal: prove the repair covers the known failure and nearby same-feature paths
without regressing backend lowering.

Actions:

- Run the supervisor-selected build command before focused tests.
- Run the focused eight-test subset named by the supervisor for the `00204`
  route, or the current equivalent focused subset.
- Add same-feature probes covering pointer-derived scalar casts and ALU
  stack-publication behavior.
- Escalate to a broader backend or full CTest checkpoint if the diff touches
  shared prepared lookup, publication-plan, or consumer-move behavior.

Completion check:

- Focused proof is green, broader proof is green when required by blast radius,
  and `todo.md` records exact commands and log locations.
