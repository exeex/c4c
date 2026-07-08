# RV64 Pointer Local-Memory Consumption Runbook

Status: Active
Source Idea: ideas/open/614_rv64_pointer_local_memory_consumption.md

## Purpose

Activate idea 614 as the RV64 consumer route for pointer/local-memory rows
whose selected pointer base+offset or pointer-value authority already exists.

## Goal

Repair RV64 local-memory consumption for selected pointer/frame-slot authority
without producing new pointer semantics or guessing missing address facts.

## Core Rule

Consume explicit selected authority from the prepared/MIR facts; do not infer
pointer base+offset, frame-slot, freshness, or memory-use facts from testcase
names, final assembly shape, or downstream behavior.

## Read First

- `ideas/open/614_rv64_pointer_local_memory_consumption.md`
- `ideas/closed/599_pointer_base_plus_offset_selected_authority.md`
- `ideas/closed/600_pointer_value_memory_use_freshness_authority.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `todo.md`

## Current Targets

- RV64 local-memory rows stopped at frame-slot or pointer base+offset
  consumption when upstream selected authority is already present.
- Rows with pointer-value memory-use freshness authority that still fail at the
  RV64 consumer boundary.
- Negative guard rows for missing selected authority, BIR GEP/address producer
  gaps, direct pointer arithmetic policy, branch/select/ABI/runtime owners,
  expectation-only changes, unsupported markers, allowlists, timeouts, and
  accounting.

## Non-Goals

- Do not repair BIR GEP/address production or selected-authority publication.
- Do not implement direct `unsupported_pointer_arithmetic` policy.
- Do not touch branch stack-source, select publication, ABI, runtime,
  expectations, unsupported markers, allowlists, timeouts, or accounting.
- Do not add filename-, row-, operation-id-, or testcase-shaped shortcuts.

## Working Model

- The source idea estimates `27` local-memory frame-slot or pointer
  base+offset rows.
- Ideas 599 and 600 provide the authority boundary this route must consume.
- Rows missing selected pointer/local-memory authority are not implementation
  targets for this idea; they should remain fail-closed or be split to their
  producer owner.

## Execution Rules

- Start each code-changing packet from refreshed backend diagnostics or
  focused probes that name first owner, selected authority facts, positive
  rows, and negative guards.
- Record the selected row family, representative positives, guard rows, and
  exact proof command in `todo.md` before implementation.
- Each code-changing step must run at least:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- Escalate to reviewer if a diff weakens unsupported contracts, rewrites
  expectations, produces upstream pointer/address authority, consumes missing
  authority, or proves progress only by matching a named source file.

## Step 1: Refresh Pointer Local-Memory Ownership

Goal: identify current pointer/local-memory rows that still belong to idea 614.

Primary target:
- Backend diagnostics and focused probes for local-memory frame-slot,
  pointer base+offset, and pointer-value memory-use consumer stops.

Actions:
- Refresh or inspect current backend diagnostics for local-memory frame-slot
  and pointer base+offset candidates.
- Split candidates by first owner:
  - RV64 local-memory consumer with complete selected pointer authority
  - RV64 pointer-value memory-use consumer with complete freshness authority
  - missing selected authority or BIR GEP/address producer gap
  - direct pointer arithmetic policy or unrelated branch/select/ABI/runtime
    owner
- Pick the first implementation packet with same-family breadth and clear
  guard rows.
- Record selected Step 2 packet, positives, negatives, and proof command in
  `todo.md`.

Completion check:
- `todo.md` names the refreshed ownership groups, selected Step 2 family,
  positive rows or a no-breadth blocker, negative guard rows, and exact proof
  command.

## Step 2: Implement First Selected Local-Memory Consumer

Goal: lower the first general RV64 local-memory consumer family that has
complete selected pointer/frame-slot authority.

Primary target:
- RV64/MIR local-memory consumer path selected by Step 1.

Actions:
- Locate the RV64 consumer rejection for the selected local-memory family.
- Add the smallest semantic lowering rule shared by the selected rows.
- Require explicit selected base, offset, frame-slot, width, and freshness
  facts appropriate to the family.
- Preserve fail-closed diagnostics for missing selected authority, producer
  gaps, pointer arithmetic policy, unsupported widths, and unrelated fragment
  classes.

Completion check:
- Multiple pointer/local-memory rows compile or move past their RV64 consumer
  stop.
- Negative guard rows remain under their original owners.
- Backend subset proof passes.

## Step 3: Broaden Within Selected Pointer Authority

Goal: extend support only to adjacent local-memory shapes that share the
proven selected-authority model.

Actions:
- Re-run focused residual probes after Step 2.
- Identify adjacent selected pointer/local-memory shapes with complete
  authority and more than testcase-only breadth when possible.
- Add narrowly scoped consumer support for same-authority variants.
- Keep BIR producer repair, direct pointer arithmetic policy, branch/select
  ownership, ABI ownership, runtime, and expectation changes out of scope.

Completion check:
- At least one adjacent local-memory row family moves past its RV64 consumer
  stop when available.
- Step 2 positives remain green.
- Negative proof still covers missing-authority and policy rows.
- Backend subset proof passes.

## Step 4: Residual Split Or Close-Readiness Classification

Goal: decide whether idea 614 is complete, needs another selected-authority
consumer packet, or should split residuals into separate open ideas.

Actions:
- Refresh remaining pointer/local-memory residuals.
- Classify residuals by first owner and selected-authority completeness.
- Identify durable follow-up ideas needed for producer gaps, direct pointer
  arithmetic policy, branch/select/ABI/runtime owners, or downstream
  consumers.
- Record close readiness, next-family recommendation, or split need in
  `todo.md`.

Completion check:
- `todo.md` states whether idea 614 is close-ready, should continue with a
  named local-memory consumer family, or should split/retire the route.
- The recommendation is backed by current diagnostics and backend subset proof
  when code changed in the route.
