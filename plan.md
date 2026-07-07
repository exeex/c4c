# BIR Scalar Binop Semantic Producer Admission Runbook

Status: Active
Source Idea: ideas/open/562_bir_scalar_binop_semantic_producer_admission.md

## Purpose

Activate the scalar-binop producer lane split from the earlier combined
semantic admission route.

## Goal

Repair or prove BIR scalar-binop semantic producer admission at the real
instruction-lowering boundary without claiming broad scalar progress from a
single representative.

## Core Rule

Treat scalar binary opcode and operand lowering as the owned surface. Do not
claim progress through expectation rewrites, unsupported downgrades, allowlists,
outer `latest function failure` note edits, scalar-control-flow repair,
function-signature repair, or downstream RV64 lowering.

## Read First

- `ideas/open/562_bir_scalar_binop_semantic_producer_admission.md`
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp`
- `src/backend/bir/lir_to_bir/scalar.cpp`
- The implementation of `lower_scalar_or_local_memory_inst()`
- The implementations of `lower_scalar_binary_opcode()` and
  `lower_scalar_binop_operands()`
- Existing backend BIR semantic admission tests that cover scalar binary
  opcodes, operand facts, or fail-closed scalar diagnostics.

## Current Scope

- Scalar-binop instruction producer behavior before `bir::BinaryInst`
  emission.
- Opcode and operand fact publication in the scalar helpers called from
  `lower_scalar_or_local_memory_inst()`.
- Representative RV64 scalar-binop row `src/960513-1.c` function `f`, or a
  current stronger substitute if the inventory has moved.
- Nearby same-feature scalar-binop rows identified by refreshed evidence before
  claiming family progress.

## Non-Goals

- Scalar-control-flow CFG, terminator, or phi-lowering repair.
- Function-signature return-info or parameter-layout repair.
- RV64 lowering or object-emission repair after BIR scalar-binop facts are
  correct.
- Classification-only, expectation-only, unsupported-marker, or allowlist
  changes.
- Broad scalar rewrites that leave the same scalar-binop admission failure in
  place.

## Working Model

The common `latest function failure` module note is a failure publication
funnel, not the owned producer. Scalar-binop failures are produced inside the
instruction-lowering path where `lower_scalar_or_local_memory_inst()` delegates
to scalar opcode and operand helpers before emitting `bir::BinaryInst`. This
runbook should isolate that boundary first, then repair only confirmed
scalar-binop fact publication gaps.

## Execution Rules

- Keep routine progress, representative diagnostics, and proof output in
  `todo.md`.
- Keep implementation changes narrowly tied to scalar binary opcode or operand
  producer behavior.
- Add focused BIR coverage for semantic fact publication or fail-closed
  scalar-binop admission before claiming producer progress.
- If a representative advances to scalar-cast, local-memory, RV64, or another
  downstream owner boundary, record that boundary in `todo.md`; do not expand
  this plan into that downstream work.
- For code-changing steps, run:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

## Step 1: Refresh Scalar-Binop Evidence

Goal: Determine the current scalar-binop producer boundary and whether existing
code already satisfies part of the source idea.

Actions:
- Inspect focused backend BIR tests for scalar-binop producer coverage.
- Re-run direct diagnostics for `src/960513-1.c` function `f`, or a current
  stronger scalar-binop substitute.
- Identify nearby same-feature scalar-binop rows before claiming family
  progress.
- Distinguish opcode failures, operand failures, outer module failure
  publication, and downstream owner boundaries.
- Run the delegated backend proof command and refresh `test_after.log`.
- Record the representative set, current owner boundary, and next packet
  recommendation in `todo.md`.

Completion Check:
- `todo.md` names the refreshed representative set and current producer or
  downstream owner boundary.
- `test_after.log` contains a fresh backend proof result.
- The next packet is either Step 2 for a real scalar-binop producer repair or
  Step 3 for broader validation and closure handoff.

## Step 2: Repair Scalar-Binop Producer Boundary

Goal: Fix only a confirmed remaining scalar-binop semantic producer failure.

Actions:
- Work in the real scalar-binop producer path, including
  `lower_scalar_or_local_memory_inst()`, `lower_scalar_binary_opcode()`, and
  `lower_scalar_binop_operands()`.
- Add or tighten focused BIR coverage for scalar binary opcode or operand fact
  publication, or for explicit fail-closed owner-boundary rejection.
- Avoid named-case shortcuts for `src/960513-1.c`, function `f`, or any single
  operand shape.
- Prove the focused test and the backend subset.
- Update `todo.md` with the changed producer rule, proof command, and any
  downstream owner boundary discovered.

Completion Check:
- The original scalar-binop semantic admission diagnostic is repaired or
  replaced by an explicit fail-closed owner-boundary diagnostic at the real
  producer.
- Focused BIR coverage demonstrates the behavior.
- Backend proof is green, and no expectation/unsupported/allowlist downgrade
  was used.

## Step 3: Broader Validation And Closure Handoff

Goal: Decide whether the source idea is complete after refreshed evidence and
any needed producer repair.

Actions:
- Run the supervisor-delegated broader validation command, or at minimum the
  backend subset if no broader command is delegated.
- Confirm that remaining failures, if any, are downstream owner boundaries and
  not scalar-binop semantic producer failures.
- Record closure evidence and residual risks in `todo.md`.
- Ask the plan owner to decide whether to close, continue, or split a new open
  idea.

Completion Check:
- `todo.md` contains enough current proof for a close decision.
- The source idea is either ready for close gate review or has a concrete
  remaining scalar-binop producer packet.
