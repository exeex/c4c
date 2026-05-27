# AArch64 Dispatch Value Materialization Prepared Authority Repair Runbook

Status: Active
Source Idea: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Supersedes active route: ideas/open/52_aarch64_calls_prepared_authority_repair.md is parked open after review/calls-step3-route-review.md judged the remaining `%t49` pointer-select aggregate-copy issue as dispatch/value-materialization ownership.

## Purpose

Make AArch64 dispatch value materialization consume prepared source, producer,
publication, memory, global, select-chain, and local-slot address facts instead
of reconstructing semantic value authority locally.

## Goal

Repair the current `00204` post-join `%t49` pointer/select aggregate-copy
materialization path without widening the calls/variadic route: `addr %t49+N`
byte-copy loads must use the authoritative pointer value selected at the
`vaarg.join.39` boundary, not a scratch value rebuilt through truncated integer
moves.

## Core Rule

Prefer existing prepared publication, value-home, source-producer, memory, and
select-chain facts. Add the smallest shared query only when non-edge value
materialization cannot consume an existing prepared authority. Do not deepen
same-block producer recursion, raw value-name scans, or testcase-shaped
shortcuts.

## Read First

- `ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md`
- `review/calls-step3-route-review.md`
- `todo.md`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- prepared facts for:
  - `PreparedEdgePublicationSourceProducerLookups`
  - `PreparedEdgePublicationSourceProducer`
  - `PreparedScalarPublicationPlan`
  - `PreparedValueHome`
  - `PreparedBlockEntryPublication`
  - `PreparedMemoryAccess`
  - `PreparedAddressingFunction`
  - recovered store-source and local-slot address helpers

## Current Targets

- `emit_value_publication_to_register`
- select-chain value materialization for non-edge/non-store consumers
- pointer-valued select results consumed by aggregate-copy address loads
- `addr %t49+N` byte-copy loads in `vaarg.join.39`
- local-slot and prepared value-home fallbacks used by value publication
- current incomplete cursor/edge-copy worktree state from the calls route:
  - `src/backend/mir/aarch64/codegen/memory.cpp`
  - `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`

## Non-Goals

- Do not change AAPCS64 call staging, variadic layout constants, `bl`/`blr`
  spelling, stack cleanup, or ABI result store spelling under this route.
- Do not continue calls/variadic cursor work except to preserve and validate the
  existing incomplete worktree improvements while repairing the dispatch-owned
  materialization blocker.
- Do not push the repair back into `calls.cpp` or `variadic.cpp` unless tracing
  proves the prepared dispatch/value-materialization facts are already correct
  and the remaining bug is wholly inside calls ownership.
- Do not downgrade expectations, mark supported tests unsupported, or claim
  helper renames as capability progress.
- Do not close the parked calls or ALU ideas as part of this switch.

## Working Model

- The calls route removed the original `%9s` overflow cursor crash but did not
  become acceptance-ready: the focused proof remains 5/6 with `00204` failing
  by runtime mismatch.
- The current uncommitted `memory.cpp` change updates a va_list field from the
  loaded cursor for the generic `load field -> add stride -> store same field`
  pattern.
- The current uncommitted `dispatch_edge_copies.cpp` change prevents the
  predecessor edge copy from reloading the join pointer from the just-advanced
  `overflow_arg_area` field.
- The remaining failure occurs after that join: emitted code materializes
  pointer-derived scratch state for `%t49` through 32-bit moves such as
  `mov w9, w13; sxtw x9, w9`, then later aggregate-copy byte loads use the bad
  pointer-derived value.
- That post-join value materialization matches the dispatch value-materialization
  source idea, especially the select-chain and local-slot address authority
  gaps.

## Execution Rules

- Treat the existing `memory.cpp` and `dispatch_edge_copies.cpp` edits as
  incomplete worktree context, not an accepted calls slice.
- Preserve the fixed overflow-block shape while repairing `%t49`; do not revert
  the current cursor or edge-copy improvements unless the supervisor explicitly
  asks for a different route.
- Add or use semantic probes for pointer-valued select results feeding aggregate
  byte-copy/address loads before accepting any code slice. A green `00204` alone
  is not enough.
- Keep the proof ladder at minimum: build, focused semantic probes, the current
  focused guardrail subset including `00204`; the supervisor decides any broader
  regression checkpoint.
- If the repair requires a broad contract shared by calls, dispatch edge copies,
  and value materialization, stop for lifecycle split instead of hiding that
  contract inside one helper.

## Steps

### Step 1: Establish pointer-select aggregate-copy baseline and probes

Goal: make the current `%t49` failure and same-feature proof surface explicit
before more code changes.

Primary target: `todo.md` proof state, focused dumps for `00204`, and semantic
backend probes under `tests/backend/case/` if no existing probes cover this
shape

Actions:

- Reuse or rerun the supervisor-selected focused subset that currently passes
  5/6 and fails only `c_testsuite_aarch64_backend_src_00204_c`.
- Dump the post-join BIR, prepared BIR, and assembly around `vaarg.join.39`,
  `%t49`, and the subsequent `addr %t49+N` byte-copy loads.
- Add or identify a minimal backend probe for a pointer-valued select result
  feeding byte-copy or address-offset loads.
- Add or identify a nearby variadic aggregate-copy probe that distinguishes the
  fixed cursor path from the remaining post-join pointer materialization path.
- Record whether prepared value-home, block-entry publication, edge-publication
  source, or select-chain facts already name the authoritative pointer source.

Completion check:

- `todo.md` records the first bad materialization fact, the expected prepared
  authority, and at least one same-feature probe that can fail or pass for the
  same semantic reason as `00204`.

### Step 2: Audit non-edge select-chain value materialization authority

Goal: decide whether `%t49` can consume existing prepared facts or needs one
small shared query.

Primary target: `dispatch_value_materialization.cpp`

Actions:

- Trace `emit_value_publication_to_register` for pointer-valued selects and
  address-offset consumers after a join block.
- Inspect whether `PreparedEdgePublicationSourceProducer`,
  `PreparedScalarPublicationPlan`, `PreparedValueHome`, or
  `PreparedBlockEntryPublication` already carries the selected pointer source.
- Identify any same-block producer recursion, raw result-name scan, or fallback
  stack-home reload that reconstructs the pointer source locally.
- If existing facts are insufficient, specify the smallest prepared
  scalar-materialization or select-chain query needed by non-edge consumers.

Completion check:

- The next code packet has one bounded authority target and no need to infer
  source truth from raw BIR spelling or narrow testcase shape.

### Step 3: Repair pointer-valued select result materialization

Goal: publish pointer-valued select results and address-offset consumers from
prepared authority without truncating pointer state.

Primary target: `dispatch_value_materialization.cpp`, plus shared prepared query
implementation only if Step 2 proves it is missing

Actions:

- Route pointer-valued select materialization through the chosen prepared
  source/home/producer fact.
- Ensure `addr %t49+N` byte-copy loads consume the full pointer value and never
  rebuild it through 32-bit scratch moves.
- Preserve existing scratch-order and read/write hazard checks while replacing
  semantic source selection.
- Keep global-load, load-local, and scalar non-pointer materialization behavior
  unchanged unless the same prepared query deliberately covers them.

Completion check:

- Semantic probes and `00204` no longer fail from pointer truncation or stale
  post-join materialization; no expectation downgrades or named-case branches
  were introduced.

### Step 4: Validate the combined unfinished code slice

Goal: determine whether the current worktree changes can become one coherent
accepted slice or need another lifecycle split.

Primary target: supervisor-selected proof logs and `todo.md`

Actions:

- Run the exact focused subset chosen by the supervisor, including the two ALU
  prepared-authority probes, `00164`, `00176`, `00181`, `00204`, and the new or
  selected semantic probes.
- Confirm the prior overflow-block cursor shape remains repaired.
- Confirm the edge-copy path still keeps the original cursor live into the join.
- Ask for reviewer scrutiny before acceptance if the final diff depends on
  inline assembly, touches multiple ownership surfaces, or claims progress only
  through `00204`.

Completion check:

- The code slice has fresh build/test proof and a recorded decision: commit as
  one coherent dispatch/value-materialization slice, split again, or revert only
  by explicit supervisor direction.

### Step 5: Continue dispatch value-materialization cleanup

Goal: reduce the duplicate authority listed in the source idea after the `%t49`
blocker is resolved.

Primary target: `dispatch_value_materialization.cpp`

Actions:

- Route same-block named producer materialization through prepared producer or
  value-home facts.
- Route load-local source materialization through prepared memory and recovered
  store-source facts.
- Route global-load materialization through prepared address/materialization
  policy if available, or add a narrow shared query.
- Route local-slot address publication through the shared frame-offset authority
  chosen by prior publication repairs.

Completion check:

- Non-edge value materialization paths no longer use recursive local recovery as
  durable semantic authority for the source-idea acceptance criteria.

### Step 6: Close or park the dispatch route

Goal: decide whether the source idea is complete or whether a separate
initiative remains.

Primary target: `plan.md`, `todo.md`, and supervisor-selected validation logs

Actions:

- Run the supervisor-selected broader validation for the dispatch
  value-materialization route.
- Request reviewer scrutiny if any slice claims progress through a narrow named
  case, expectation rewrite, or retained raw producer scan.
- Close only if the source idea acceptance criteria are satisfied; otherwise
  leave a bounded follow-up packet or ask the plan owner to split a distinct
  initiative.

Completion check:

- The source idea is either closed with regression proof or remains open with a
  precise next packet that does not hide route drift.
