# Before-Instruction Stack-To-Register Move Materialization Plan

Status: Active
Source Idea: ideas/open/457_before_instruction_stack_to_register_move_materialization.md

## Purpose

Classify and materialize the next before-instruction move-bundle family exposed
after the explicit cast-dependency consumer work closed.

## Goal

Audit the `20010329-1` `block_index=4 instruction_index=2` move bundle and
select only a bounded RV64 materialization packet for explicit prepared
stack/register-source moves into register destinations.

## Core Rule

Do not infer move materialization from raw BIR shape or representative layout.
RV64 may consume only explicit prepared before-instruction move-bundle facts,
with source/destination homes and scratch/clobber safety proven.

## Read First

- ideas/open/457_before_instruction_stack_to_register_move_materialization.md
- ideas/closed/456_rv64_select_edge_cast_dependency_consumer.md
- build/agent_state/456_step7_final_residual_disposition/disposition.md
- build/agent_state/456_step7_final_residual_disposition/20010329-1.prepared.out
- build/agent_state/456_step7_final_residual_disposition/20010329-1.object.err
- src/backend/mir/riscv/codegen/object_emission.cpp
- src/backend/prealloc/prepared_object_traversal.cpp
- src/backend/prealloc/prepared_object_traversal.hpp
- tests/backend/mir/backend_riscv_object_emission_test.cpp

## Current Targets

- `20010329-1` current `unsupported_move_bundle_target_shape` residual.
- Before-instruction move bundle at `block_index=4 instruction_index=2`.
- Move `from_value_id=7 to_value_id=10 destination_storage=register`.
- Move `from_value_id=9 to_value_id=10 destination_storage=register
  reason=consumer_stack_to_register`.
- Source/destination homes, carrier use, and scratch/clobber conditions for
  this register-destination materialization family.

## Non-Goals

- Reopening explicit cast-dependency authority consumption closed by idea 456.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Generic stack-home branch operand or condition materialization tracked by
  idea 451.
- Pointer-value memory provenance publication, local/global store publication,
  or generic instruction-side lowering.
- Raw-BIR reconstruction or policy inference from filenames, function names,
  block names, testcase names, value ids alone, or one prepared dump.
- Expectation rewrites, unsupported downgrades, allowlists, runtime-comparison
  changes, pass/fail accounting changes, or `test_baseline.new.log` changes.
- Touching `review/`, `test_before.log`, or `test_after.log`.

## Working Model

Idea 456 removed the explicit cast-dependency blocker and the authorized
stack-publication blocker. The next residual is a later before-instruction
move bundle that needs ordinary move materialization into a register
destination. This plan first classifies the bundle and then selects a narrow
consumer packet only if the prepared facts carry enough authority and safety.

## Execution Rules

- Start with evidence classification; do not edit implementation in Step 1.
- Treat prepared move-bundle rows as candidate authority; verify source homes,
  destination homes, carrier use, and scratch/clobber safety before selecting
  code.
- Keep cast-dependency authority, stack-load freshness, pointer-provenance, and
  generic branch stack-home work separate.
- Add focused RV64 object tests for accepted materialization and fail-closed
  unsupported or unsafe move bundles.
- Select at most one narrow RV64 consumer packet after the audit is explicit.
- Do not touch `test_baseline.new.log`, `test_before.log`, `test_after.log`,
  or `review/`.
- Classification-only proof: `git diff --check`.
- Code/test proof, if implementation is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Before-Instruction Register-Destination Move Bundle

Inspect the 456 Step 7 artifacts for `20010329-1`. Record each move in
`block_index=4 instruction_index=2`, source value, destination value, source
home, destination home, reason, carrier use, scratch/clobber risk, traversal
event, and first missing authority. Completion means `todo.md` contains a
bucket table and identifies the first bounded materialization packet or exact
blocker.

### Step 2: Define Register-Destination Move Materialization Contract

Specify accepted and rejected before-instruction move shapes for this family,
including stack-to-register and register-to-register source handling,
destination register constraints, ordering, scratch usage, and fail-closed
adjacent cases. Completion means `todo.md` states owned files/tests and proof
command.

### Step 3: Implement Or Route First Move-Bundle Packet

If a coherent RV64 consumer packet exists, implement the smallest semantic
change with focused coverage. If the first owner is missing producer facts,
scratch allocation, pointer provenance, generic stack-home materialization, or
another family, record the split or blocker instead of broadening this source.
Completion means proof passes or canonical lifecycle state records the route
decision.

### Step 4: Residual Disposition And Close Readiness

Re-check `20010329-1` and focused coverage against the Step 3 result, classify
remaining residuals, and decide whether this source idea is complete.
Completion means close, keep active with one exact remaining packet, or route
durable follow-up work.
