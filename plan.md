# RV64 Preterminator Predecessor-Edge Parallel-Copy Materialization Plan

Status: Active
Source Idea: ideas/open/462_rv64_preterminator_predecessor_edge_parallel_copy_materialization.md
Activated From: ideas/closed/461_rv64_move_bundle_coordinate_diagnostics.md

## Purpose

Classify and, if justified, implement RV64 object-route handling for the
coordinate-proven `pre_terminator_copies` predecessor-edge parallel-copy
residual owner.

## Goal

Advance the first `20010329-1` `unsupported_move_bundle_target_shape` owner
using the object-route coordinate evidence for the `logic.rhs.end.40 ->
logic.end.41` out-of-SSA parallel copy, without falling back to raw-shape or
testcase-specific inference.

## Core Rule

Only act on coordinate/authority-backed prepared facts for the proven
`out_of_ssa_parallel_copy` register-destination event. Keep generic
move-bundle support, stale stack-load consumption, and expectation changes out
of scope.

## Read First

- ideas/open/462_rv64_preterminator_predecessor_edge_parallel_copy_materialization.md
- ideas/closed/461_rv64_move_bundle_coordinate_diagnostics.md
- build/agent_state/461_step4_residual_disposition/disposition.md
- build/agent_state/461_step3_coordinate_diagnostic_packet/20010329-1.object.err
- build/agent_state/460_step1_move_bundle_residual_audit/20010329-1.prepared.out
- src/backend/mir/riscv/codegen/object_emission.cpp

## Current Target

Coordinate-bearing first failure for `20010329-1`:

- event kind: `pre_terminator_copies`
- function/block: `main`, `block_index=10`, `block_label=logic.rhs.end.40`
- phase: `block_entry`
- authority: `out_of_ssa_parallel_copy`
- parallel copy: `logic.rhs.end.40 -> logic.end.41`
- execution site: `predecessor_terminator`
- move: `move[0].from_value_id=20` to `move[0].to_value_id=21`
- destination: `destination_storage=register`
- reason: `phi_join_register_to_register`
- excluded authorities: `select_edge_suppression_authorized=no`,
  `cast_dependency_stack_publication_authorized=no`
- current fragment status: `generic_move_bundle_materialization_failed`

## Non-Goals

- Generic stack-to-register, register-to-register, or all-purpose move-bundle
  support.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Reopening ideas 456, 458, 459, 460, or 461 without new coordinate-bearing
  evidence.
- Ownership inference from prepared dump order, raw BIR shape, value ids
  alone, block labels alone, function names, filenames, or testcase name.
- Expectation rewrites, unsupported downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

The diagnostic/probe work is complete. The next question is whether the
coordinate-proven predecessor-edge out-of-SSA parallel copy has enough
prepared authority and value-home information to materialize a narrow RV64
register-destination move, or whether a producer/prepared metadata blocker
must be split before implementation.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Any implementation must consume explicit coordinate/authority-backed facts,
  not raw BIR shape or testcase identity.
- Preserve fail-closed behavior for raw-only, missing-authority, mismatched
  edge, non-register destination, stale stack-load, and generic move cases.
- Classification-only proof:

```sh
git diff --check
```

- Code/test proof, if a consumer packet is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Coordinate-Bearing Parallel-Copy Evidence

Re-read the 461 disposition, fresh object stderr, prepared output, and relevant
RV64 move-bundle traversal. Record the exact prepared event, value homes,
source/destination availability, edge identity, execution site, and authority
facts for the `logic.rhs.end.40 -> logic.end.41` copy. Completion means
`todo.md` contains an audit table and classifies whether Step 2 can define a
consumer contract or must route to a producer/prepared metadata blocker.

### Step 2: Define Preterminator Parallel-Copy Materialization Contract

Specify the accepted route for the coordinate-proven event: required
authority, phase, edge identity, destination storage, source availability,
emission point, and fail-closed cases. Completion means `todo.md` records the
contract, target files/tests if implementation is justified, or the exact
blocker if no sound consumer packet exists.

### Step 3: Implement Or Route First Parallel-Copy Packet

If Step 2 finds a bounded RV64 consumer packet, implement only that packet with
focused coverage and proof. If no implementation is justified, record the
precise blocker and route it to a separate source idea. Completion means proof
passes for the selected packet or lifecycle state records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-probe the representative object route and classify any remaining first
owner. Completion means the source idea closes, remains active with one exact
in-scope packet, or splits a separate durable follow-up.
