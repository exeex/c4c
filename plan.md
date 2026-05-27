# AArch64 Edge Terminator Consumer Preservation Repair Runbook

Status: Active
Source Idea: ideas/open/56_aarch64_edge_terminator_consumer_preservation_repair.md
Supersedes active route: ideas/open/55_aarch64_scalar_cast_alu_publication_prepared_authority_repair.md is parked open after Step 3 classified the remaining stale-source failure as edge/terminator preservation placement, not scalar cast ownership.

## Purpose

Make prepared edge and terminator publication preserve successor consumer
sources before out-of-SSA edge copies reuse the same physical register for a
different value.

## Goal

Repair the remaining `00204` mismatch where `%t35` is still needed for
`%t45.byte_offset.i64`, but predecessor edge publication has already made `x13`
authoritative for `%t49` before the join-block consumer move runs.

## Core Rule

Prepared planning must place or expose preservation before the clobbering edge
publication. AArch64 lowering must not recover the source from raw BIR spelling,
same-block scans, mutable local reloads, or stale emitted-register maps.

## Read First

- `ideas/open/56_aarch64_edge_terminator_consumer_preservation_repair.md`
- `todo.md`
- `src/backend/prealloc/regalloc/consumer_moves.cpp`
- `src/backend/prealloc/regalloc/consumer_moves.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- current incomplete context:
  - `src/backend/mir/aarch64/codegen/memory.cpp`
  - `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`

## Current Targets

- `%t35 -> %t45.byte_offset.i64` consumer preservation
- `%t45` / `%t47 -> %t49` predecessor edge publication
- predecessor-side placement before `x13` is repurposed for `%t49`
- prepared lookup exposure only if lowering needs to consume the preserved
  source explicitly

## Non-Goals

- Do not reload `%lv.ap.24` in `vaarg.join.39`; the va_list field has already
  been updated.
- Do not continue with a cast-only or ALU-only patch that has no valid current
  `%t35` source.
- Do not change AAPCS64 call staging, variadic layout constants, or cursor
  writeback rules under this route.
- Do not accept or commit the dirty `memory.cpp` or `dispatch_edge_copies.cpp`
  changes as part of this lifecycle reset.
- Do not downgrade expectations, mark supported tests unsupported, or claim
  helper renames as semantic progress.

## Working Model

- Earlier dirty calls/memory work removed the original `00204` segfault, but is
  still unaccepted context.
- Earlier dirty edge-copy work removed the bad reload of the selected pointer
  before `vaarg.join.39`, but is still unaccepted context.
- Scalar cast classification proved the bad `mov w9, w13; sxtw x9, w9` is a
  symptom: cast lowering sees stale `%t35` in `x13`, while `x13` now belongs to
  `%t49`.
- Prepared BIR already knows the consumer move `%t35 -> %t45.byte_offset.i64`,
  but the move is scheduled at the join instruction, after predecessor edge
  publication has clobbered the source register.
- The next semantic owner is prepared edge/terminator preservation placement.

## Execution Rules

- Treat existing `memory.cpp` and `dispatch_edge_copies.cpp` edits as
  incomplete worktree context unless the supervisor separately accepts or
  reroutes them.
- Start from prepared planning and edge publication order. Do not start by
  patching `cast_ops.cpp`.
- Prefer a general rule for successor consumer preservation before predecessor
  edge-copy clobbers.
- Keep the proof ladder at minimum:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`
- A green `00204` alone is not enough; the same-feature probes must stay in the
  subset and no expectation may be weakened.

## Steps

### Step 1: Establish edge preservation baseline

Goal: prove the exact prepared move and edge-publication facts that conflict.

Primary target: `todo.md` classification and focused dumps

Actions:

- Run the eight-test focused subset.
- Dump prepared BIR around `%t35`, `%t45.byte_offset.i64`, `%t49`, and
  `vaarg.join.39`.
- Trace where the `%t35 -> %t45.byte_offset.i64` consumer move is planned and
  where `%t45` / `%t47 -> %t49` edge publication is planned.
- Record the current ordering and the physical-register clobber that makes the
  join-time move stale.

Completion check:

- `todo.md` names the prepared data structures and functions that own both the
  late consumer move and the predecessor edge clobber.

### Step 2: Define the preservation contract

Goal: choose the smallest prepared contract that can express successor consumer
preservation before predecessor edge publication clobbers the source register.

Primary target: `src/backend/prealloc/regalloc/consumer_moves.cpp` and
`src/backend/prealloc/publication_plans.cpp`

Actions:

- Inspect whether consumer moves can be scheduled on predecessor terminators or
  edge bundles when the successor placement is too late.
- Decide whether the contract belongs in consumer-move planning, edge
  publication planning, or a shared prepared lookup exposed to both.
- Reject value-name-only and testcase-shaped placement rules.
- Keep the contract independent of `00204` temporary names.

Completion check:

- The chosen contract is recorded in `todo.md` with exact owned files and the
  next implementation packet.

### Step 3: Implement predecessor-side preservation placement

Goal: preserve or materialize the needed successor consumer source before the
edge publication that would overwrite its register.

Primary target: prepared planning plus `dispatch_edge_copies.cpp` only if codegen
must emit the prepared edge preservation.

Actions:

- Add or extend prepared data so the predecessor edge can carry the preservation
  move before the clobbering publication.
- Ensure the ordering preserves `%t35` before `x13` becomes authoritative for
  `%t49`.
- Expose a prepared lookup only if AArch64 scalar consumers must read the
  preserved source explicitly.
- Do not reload mutable locals and do not infer from raw BIR spelling.

Completion check:

- The focused subset shows `00204` has moved past the `%t35` stale-source
  failure while the two route probes still pass.

### Step 4: Audit consumers of the preserved source

Goal: make sure AArch64 cast/ALU consumers use the prepared preservation result
instead of stale emitted-register state.

Primary target: `cast_ops.cpp` and `alu.cpp` only if Step 3 exposes a consumer
query that these files must use.

Actions:

- Confirm whether the predecessor-side preservation makes existing scalar cast
  lowering correct without further changes.
- If not, consume the prepared source query in the smallest scalar publication
  path.
- Do not add a second source-selection authority in codegen.

Completion check:

- The pointer-corrupting `mov w9, w13; sxtw x9, w9` sequence is absent for a
  semantic reason, and the focused subset remains green or records the next
  owner precisely.

### Step 5: Validate and decide dirty-context acceptance

Goal: decide whether the active worktree is one coherent accepted slice or
whether dirty predecessor repairs need separate lifecycle ownership.

Primary target: `todo.md`, `test_before.log`, and `test_after.log`

Actions:

- Run the exact eight-test focused subset.
- Confirm the original `00204` segfault stays gone.
- Confirm edge publication keeps `%t49` live and also preserves `%t35` before
  clobber.
- Ask for reviewer scrutiny if the final diff spans memory, edge copies,
  prepared planning, cast, and ALU.

Completion check:

- `todo.md` records either a coherent commit-ready slice, a precise split for
  dirty predecessor repairs, or a blocker that should return to plan-owner.

### Step 6: Close or park the edge preservation route

Goal: decide whether the source idea is complete or a bounded follow-up remains.

Primary target: lifecycle files and supervisor-selected validation logs

Actions:

- Run the supervisor-selected broader validation for this route.
- Request reviewer scrutiny if progress depends mainly on `00204`.
- Close only if the source idea acceptance criteria are satisfied.

Completion check:

- The source idea is closed with regression proof, or remains open with a
  precise next packet that does not hide route drift.
