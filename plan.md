# RV64 Move-Bundle Coordinate Diagnostics Plan

Status: Active
Source Idea: ideas/open/461_rv64_move_bundle_coordinate_diagnostics.md

## Purpose

Produce coordinate-bearing evidence for the remaining RV64 move-bundle
rejection before selecting another lowering owner.

## Goal

Make the `20010329-1` `unsupported_move_bundle_target_shape` failure identify
the first rejecting move-bundle event well enough to route the next source idea
without raw-shape inference.

## Core Rule

Do not implement semantic lowering in this diagnostics/probe plan. The output
is evidence: phase, block, instruction, move identity, destination storage,
reason, authority status, and owner classification.

## Read First

- ideas/open/461_rv64_move_bundle_coordinate_diagnostics.md
- ideas/closed/460_rv64_move_bundle_residual_owner_audit.md
- build/agent_state/460_step2_residual_disposition/disposition.md
- build/agent_state/460_step1_move_bundle_residual_audit/audit.md
- build/agent_state/460_step1_move_bundle_residual_audit/20010329-1.prepared.out
- build/agent_state/460_step1_move_bundle_residual_audit/20010329-1.object.err
- src/backend/mir/riscv/codegen/object_emission.cpp

## Current Targets

- `20010329-1` object route still exits 2.
- Current object stderr only says
  `unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves`.
- Strongest unproven candidate: before-instruction stack publication at
  `block_index=4 instruction_index=1` for `%t17`.
- Other candidate families: idea-459 suppression target, later register setup
  rows, block-entry select publication copies, or another move-bundle owner.

## Non-Goals

- RV64 move-bundle lowering.
- Generic stack-to-register or register-to-register move support.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Reopening ideas 456, 458, or 459 without coordinate evidence.
- Ownership inference from raw BIR shape, filenames, function names, or one
  prepared dump alone.
- Expectation rewrites, unsupported downgrades, allowlists, runtime-comparison
  changes, pass/fail accounting changes, or `test_baseline.new.log` changes.
- Touching `review/`, `test_before.log`, or `test_after.log`.

## Working Model

The previous audit narrowed the candidate set but lacked object-route
coordinates. This plan adds enough diagnostic/probe evidence to identify the
first failing event and then routes the next idea.

## Execution Rules

- Start with evidence classification; do not edit implementation in Step 1.
- If code changes are selected later, they must be diagnostic/probe-only and
  covered by focused tests.
- Keep stale stack-load authority, generic move support, and raw-shape
  inference rejected.
- Classification-only proof: `git diff --check`.
- Code/test proof, if diagnostic implementation is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Diagnostic Coordinate Gap

Re-read the 460 artifacts and current RV64 diagnostic path. Record what
coordinate fields are missing, which candidate event families must be
distinguished, and where diagnostic/probe evidence can be added or collected.
Completion means `todo.md` contains a coordinate-gap table and identifies the
first diagnostic/probe packet or exact blocker.

### Step 2: Define Coordinate Diagnostic Contract

Specify the coordinate-bearing evidence required for move-bundle rejection
ownership: phase, block index, instruction index, move index or identity,
destination storage, reason, authority status, and owning event family.
Completion means `todo.md` states accepted diagnostic shape, owned files/tests
if code is needed, and proof command.

### Step 3: Implement Or Route First Diagnostic Packet

If a coherent diagnostic/probe implementation packet exists, implement the
smallest evidence-only change with focused coverage. If implementation is not
needed or not possible, record the blocker and route accordingly. Completion
means proof passes or canonical lifecycle state records the route decision.

### Step 4: Residual Disposition And Close Readiness

Use the coordinate-bearing evidence to classify the residual owner and decide
whether this diagnostics idea is complete. Completion means close with a
durable follow-up, keep active with one exact diagnostic packet, or record a
blocker.
