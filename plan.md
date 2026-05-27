# AArch64 Scalar Cast ALU Publication Prepared Authority Repair Runbook

Status: Active
Source Idea: ideas/open/55_aarch64_scalar_cast_alu_publication_prepared_authority_repair.md
Supersedes active route: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md is parked open after review/dispatch-step3-route-review.md judged the remaining `00204` mismatch outside the dispatch value-materialization owned surface.

## Purpose

Make AArch64 scalar cast and ALU publication paths consume prepared source/home
authority when publishing pointer-derived scalar values, including cases where
block-entry or edge publication has reused the physical register that formerly
held the scalar source.

## Goal

Repair the remaining `00204` post-join mismatch around
`%t45.byte_offset.i64` / `%t45`: the aggregate byte-copy address path must keep
selected pointer value `%t49` and prior offset source `%t35` distinct, and must
not materialize a pointer-shaped scratch through `mov w9, w13; sxtw x9, w9`.

## Core Rule

Prepared publication must either provide a valid current source for scalar
cast/ALU consumers or preserve/place that source before block-entry or edge
publication overwrites its physical register. Do not recover truth from raw BIR
spelling, same-block scans, mutable local reloads, or the current scratch
register contents.

## Read First

- `ideas/open/55_aarch64_scalar_cast_alu_publication_prepared_authority_repair.md`
- `todo.md`
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/regalloc/consumer_moves.cpp`
- `src/backend/prealloc/regalloc/consumer_moves.hpp`
- prepared facts for:
  - `PreparedValueHome`
  - `PreparedStoragePlanValue`
  - `PreparedScalarPublicationPlan`
  - `PreparedEdgePublicationSourceProducerLookups`
  - `PreparedBlockEntryPublication`
  - `PreparedMoveBundle`

## Current Targets

- scalar cast publication for `%t45.byte_offset.i64`
- preservation or placement of `%t35` after `%t49` takes over `x13` at
  `vaarg.join.39`
- shared prepared query consumed by `cast_ops.cpp` and later `alu.cpp`, only if
  preservation/placement already exists but is not exposed
- current incomplete predecessor context:
  - `src/backend/mir/aarch64/codegen/memory.cpp`
  - `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`

## Non-Goals

- Do not repair this by reloading `%lv.ap.24` at the join; that local has been
  mutated and no longer represents the original `%t35`.
- Do not continue with a cast-only patch that merely rejects stale `x13` without
  providing a valid replacement source.
- Do not alter calls/variadic layout, ABI staging, memory cursor constants, or
  edge-copy mechanics under this route.
- Do not accept the dirty `memory.cpp` or `dispatch_edge_copies.cpp` changes as
  finished work in this lifecycle rewrite.
- Do not close the parked dispatch, calls, ALU, memory, or comparison ideas as
  part of this reset.
- Do not downgrade expectations, mark supported tests unsupported, or claim
  helper renames as semantic progress.

## Working Model

- The previous calls route removed the original `00204` segfault by improving
  the va_list overflow cursor path, but the implementation remains unaccepted.
- The previous edge-copy repair prevented the join pointer from being reloaded
  from the just-advanced `overflow_arg_area`, but that change is still dirty
  context.
- The dispatch value-materialization audit found that `%t49` is already
  republished into `x13`; a direct `dispatch_value_materialization.cpp` helper
  edit did not affect the bad sequence and was reverted.
- Step 1 traced the remaining bad sequence to scalar cast publication for
  `%t45.byte_offset.i64`, through `lower_scalar_cast_publication_to_prepared_stack`
  and recursive `CastOpcode::SExt` publication for `%t35`.
- Step 2 proved a cast-only repair is unsafe: `cast_ops.cpp` can detect that
  `x13` is stale for `%t35`, but no exposed prepared fact provides the original
  `%t35` after block-entry/edge publication makes `x13` authoritative for `%t49`.
- The next owner is prepared source preservation or placement, not testcase
  matching and not a mutable local reload.

## Execution Rules

- Treat existing `memory.cpp` and `dispatch_edge_copies.cpp` edits as
  incomplete worktree context. Preserve them while diagnosing unless the
  supervisor explicitly asks for revert or reroute.
- Start with the prepared-planning question: where should `%t35` be preserved,
  or where should `%t45.byte_offset.i64` / `%t45` be materialized, so the scalar
  cast consumer has a semantically valid source?
- Keep cast/ALU emitter edits minimal until the prepared source query or
  placement exists.
- Keep the proof ladder at minimum:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`
- A green `00204` alone is not enough; the same-feature probes must stay in the
  subset and no expectation may be weakened.
- If tracing proves source preservation belongs to a separate non-scalar route,
  stop for lifecycle split instead of broadening this runbook silently.

## Steps

### Step 1: Establish scalar cast ALU publication baseline

Goal: prove the exact cast or ALU publication emitter for the remaining
pointer-corrupting sequence.

Primary target: `todo.md` proof state and focused dumps for `00204`

Actions:

- Run the supervisor-selected eight-test focused subset.
- Dump the prepared BIR and assembly around `vaarg.join.39`,
  `%t45.byte_offset.i64`, `%t45`, `%t49`, and the aggregate byte-copy loads.
- Trace which `cast_ops.cpp` or `alu.cpp` lowering function emits the
  `mov w9, w13; sxtw x9, w9` sequence.
- Record the prepared value-home, scalar-publication, block-entry, or
  source-producer fact that should identify the source register or home.

Completion check:

- `todo.md` names the first bad scalar publication emitter, the stale source it
  used, and the prepared authority conflict.

### Step 2: Reject unsafe cast-only repair

Goal: prove whether `cast_ops.cpp` already has a valid replacement source once
it detects stale register authority.

Primary target: `cast_ops.cpp` diagnosis and `todo.md`

Actions:

- Check whether `lower_scalar_cast_publication_to_prepared_stack` can replace
  stale `find_emitted_scalar_register` state with an existing prepared source.
- Distinguish true current homes from historical register homes overwritten by
  block-entry or edge publication.
- Reject mutable local reloads when the local may have changed since the source
  value was produced.

Completion check:

- `todo.md` records that the cast-only path is blocked unless prepared planning
  preserves or relocates `%t35`, or exposes a valid current source query.

### Step 3: Repair prepared source preservation or placement

Goal: give scalar cast/ALU publication a semantically valid source when a join
or edge publication reuses the source value's physical register for another
value.

Primary target: `src/backend/prealloc/publication_plans.cpp`,
`src/backend/prealloc/publication_plans.hpp`,
`src/backend/prealloc/prepared_lookups.cpp`,
`src/backend/prealloc/prepared_lookups.hpp`, and possibly
`src/backend/prealloc/regalloc/consumer_moves.cpp` /
`src/backend/prealloc/regalloc/consumer_moves.hpp`

Actions:

- Inspect prepared block-entry, edge-publication, move-bundle, and consumer-move
  facts for `%t35`, `%t45.byte_offset.i64`, `%t45`, and `%t49`.
- Choose the smallest semantic repair:
  - preserve `%t35` before `%t49` overwrites `x13`,
  - materialize `%t45.byte_offset.i64` / `%t45` before `%t35` is overwritten,
  - or expose an existing preserved source through a prepared lookup consumed
    by scalar cast/ALU publication.
- Do not add a value-name-only query or same-block producer scan.
- Do not route the repair through a join-time reload of `%lv.ap.24`.
- Update `cast_ops.cpp` only as a consumer of the new or existing prepared
  source answer if the prepared layer repair requires it.

Completion check:

- The prepared plan either emits a preservation/consumer move or exposes a
  current source for `%t35` before the scalar cast publication needs it, and
  the eight-test focused subset shows `00204` has moved past this stale-source
  failure without weakening the probes.

### Step 4: Repair ALU publication consumers if still stale

Goal: apply the same prepared source/home authority to ALU stack-publication
paths if Step 3 exposes the shared query but only cast has consumed it.

Primary target: `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Audit ALU stack-publication paths for derived scalar values using stale
  emitted register state.
- Prefer the prepared source-preservation query or placement result before
  reusing register homes.
- Keep integer ALU opcode spelling and scratch-order behavior unchanged except
  for source authority selection.

Completion check:

- ALU publication no longer republish stale registers for derived scalar values,
  and the focused subset remains green or records the next non-ALU owner.

### Step 5: Validate combined dirty context and scalar publication repair

Goal: decide whether the current worktree can become one coherent accepted
slice or whether predecessor repairs need their own lifecycle route.

Primary target: supervisor-selected proof logs and `todo.md`

Actions:

- Run the exact eight-test focused subset from the execution rules.
- Confirm the original `00204` overflow cursor segfault remains gone.
- Confirm the edge-copy path still keeps the selected pointer live into
  `vaarg.join.39`.
- Confirm the scalar cast/ALU repair does not depend on a testcase-shaped
  branch or expectation rewrite.
- Ask for reviewer scrutiny before acceptance if the final diff spans memory,
  edge copies, prepared planning, cast, and ALU.

Completion check:

- `todo.md` records one of: commit as a coherent proven slice, split dirty
  predecessor repairs into their own owner route, or stop for supervisor
  direction.

### Step 6: Close or park the scalar cast ALU route

Goal: decide whether the source idea is complete or whether a bounded follow-up
remains.

Primary target: `plan.md`, `todo.md`, and supervisor-selected validation logs

Actions:

- Run the supervisor-selected broader validation for this route.
- Request reviewer scrutiny if the final slice spans multiple ownership
  surfaces or claims progress mainly through `00204`.
- Close only if the source idea acceptance criteria are satisfied; otherwise
  leave a precise next packet or ask the plan owner to split a distinct
  initiative.

Completion check:

- The source idea is either closed with regression proof or remains open with a
  next packet that does not hide route drift.
