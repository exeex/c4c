# AArch64 Scalar Cast ALU Publication Prepared Authority Repair Runbook

Status: Active
Source Idea: ideas/open/55_aarch64_scalar_cast_alu_publication_prepared_authority_repair.md
Supersedes active route: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md is parked open after review/dispatch-step3-route-review.md judged the remaining `00204` mismatch outside the dispatch value-materialization owned surface.

## Purpose

Make AArch64 scalar cast and ALU publication paths consume prepared source/home
authority when publishing pointer-derived scalar values, so a live pointer
register is not reinterpreted as a stale 32-bit offset source.

## Goal

Repair the remaining `00204` post-join mismatch around
`%t45.byte_offset.i64` / `%t45`: the aggregate byte-copy address path must keep
the selected pointer value and integer offset source distinct, and must not
materialize a pointer-shaped scratch through `mov w9, w13; sxtw x9, w9`.

## Core Rule

Prefer prepared value-home, scalar-publication, source-producer, block-entry,
and edge-publication facts for scalar cast/ALU source selection. Add one small
shared query only if existing prepared facts cannot serve both `cast_ops.cpp`
and `alu.cpp`. Do not recover truth from raw BIR spelling, same-block scans, or
the current scratch register contents.

## Read First

- `ideas/open/55_aarch64_scalar_cast_alu_publication_prepared_authority_repair.md`
- `review/dispatch-step3-route-review.md`
- `todo.md`
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- prepared facts for:
  - `PreparedValueHome`
  - `PreparedStoragePlanValue`
  - `PreparedScalarPublicationPlan`
  - `PreparedEdgePublicationSourceProducerLookups`
  - `PreparedBlockEntryPublication`
  - `PreparedMoveBundle`

## Current Targets

- scalar cast publication for pointer-derived values
- ALU stack publication for derived scalar values
- source selection for `%t45.byte_offset.i64` / `%t45`
- register-home freshness where join-block publication has already placed the
  selected pointer in `x13`
- current incomplete predecessor context:
  - `src/backend/mir/aarch64/codegen/memory.cpp`
  - `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`

## Non-Goals

- Do not continue implementation under the dispatch value-materialization route
  unless tracing proves a shared query must be exposed for cast/ALU consumers.
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
- The remaining bad sequence appears later in ordinary scalar cast/ALU
  publication for `%t45.byte_offset.i64` / `%t45`, where `x13` is treated as an
  integer offset source and narrowed through `w13`.
- That ownership surface spans `cast_ops.cpp` and `alu.cpp`, so it needs a
  separate source idea instead of widening idea 49.

## Execution Rules

- Treat existing `memory.cpp` and `dispatch_edge_copies.cpp` edits as
  incomplete worktree context. Preserve them while diagnosing unless the
  supervisor explicitly asks for revert or reroute.
- Begin with classification/proof of the exact emitter for the bad scalar
  publication before changing code.
- Keep the proof ladder at minimum:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`
- A green `00204` alone is not enough; the same-feature probes must stay in the
  subset and no expectation may be weakened.
- If tracing proves the true owner is outside cast/ALU publication, stop for a
  lifecycle split instead of broadening this runbook silently.

## Steps

### Step 1: Establish scalar cast ALU publication baseline

Goal: prove the exact cast or ALU publication emitter for the remaining
pointer-corrupting sequence before any more implementation.

Primary target: `todo.md` proof state and focused dumps for `00204`

Actions:

- Rerun or reuse the supervisor-selected eight-test focused subset.
- Dump the prepared BIR and assembly around `vaarg.join.39`,
  `%t45.byte_offset.i64`, `%t45`, `%t49`, and the aggregate byte-copy loads.
- Trace which `cast_ops.cpp` or `alu.cpp` lowering function emits the
  `mov w9, w13; sxtw x9, w9` sequence.
- Record the prepared value-home, scalar-publication, block-entry, or
  source-producer fact that should identify the source register or home.
- Confirm whether `dispatch_value_materialization.cpp` is only an upstream
  producer or must expose a shared query for this cast/ALU consumer.

Completion check:

- `todo.md` names the first bad scalar publication emitter, the stale source it
  used, the prepared authority it should consume, and whether the next packet
  is cast-only, ALU-only, or a small shared-query repair.

### Step 2: Repair scalar cast source selection

Goal: make cast lowering consume the authoritative full-width source for
pointer-derived scalar publications.

Primary target: `cast_ops.cpp`, plus shared prepared lookup code only if Step 1
proves no existing query is sufficient

Actions:

- Replace stale live-register or raw producer recovery with the prepared source
  selected in Step 1.
- Preserve correct sign/zero-extension semantics for true integer casts.
- Keep pointer-derived publications from narrowing through `w` registers unless
  the source type and prepared facts prove a real 32-bit integer value.
- Add a focused probe only if existing probes cannot distinguish pointer-source
  preservation from integer cast spelling.

Completion check:

- Cast lowering no longer corrupts pointer-derived scalar sources, and focused
  proof shows no expectation downgrade or named-case branch.

### Step 3: Repair ALU stack-publication source selection

Goal: make ALU publication choose prepared homes/sources for derived scalar
values instead of republishing stale register contents.

Primary target: `alu.cpp`, plus shared prepared lookup code only if Step 1 or
Step 2 proves the query belongs outside one file

Actions:

- Audit the ALU stack-publication path for `%t45.byte_offset.i64` / `%t45`.
- Prefer prepared value-home, storage-plan, scalar-publication, or
  source-producer facts before reusing register homes.
- Keep integer ALU opcode spelling and scratch-order behavior unchanged except
  for source authority selection.
- Ensure pointer and offset sources remain distinct across join-block
  publication.

Completion check:

- The bad `mov w9, w13; sxtw x9, w9` sequence is removed for the correct
  semantic reason, and the focused subset no longer fails from this stale
  scalar-publication path.

### Step 4: Validate combined dirty context and scalar publication repair

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
  edge copies, cast, and ALU.

Completion check:

- `todo.md` records one of: commit as a coherent proven slice, split dirty
  predecessor repairs into their own owner route, or stop for supervisor
  direction.

### Step 5: Continue cast ALU prepared-authority cleanup

Goal: reduce remaining duplicate scalar cast/ALU publication authority in the
new source idea after the `00204` blocker is resolved.

Primary target: `cast_ops.cpp` and `alu.cpp`

Actions:

- Remove or route any remaining raw source-selection scans found during Step 1.
- Share the chosen prepared scalar-publication query between cast and ALU paths
  where both need the same source answer.
- Keep target-local opcode selection, scratch hazards, and extension semantics
  local to their emitters.

Completion check:

- Cast and ALU publication paths use prepared source/home facts for semantic
  source selection, with any remaining target-local logic clearly separated.

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
