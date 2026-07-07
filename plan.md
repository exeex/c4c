# BIR Scalar Control Flow Semantic Producer Admission Runbook

Status: Active
Source Idea: ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md

## Purpose

Re-activate the scalar-control-flow producer lane and decide whether the open
idea is now closable or still needs a focused producer repair.

## Goal

Prove the current scalar-control-flow BIR semantic admission boundary with
focused evidence, then either repair any real producer regression or hand the
idea back for closure.

## Core Rule

Treat scalar-control-flow producer facts as the owned surface. Do not claim
progress through expectation rewrites, unsupported downgrades, allowlists,
outer `latest function failure` note edits, RV64 object lowering, function
signature lowering, scalar-binop lowering, or local-memory work.

## Read First

- `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`
- `src/backend/bir/lir_to_bir/module.cpp`
- Existing backend BIR semantic admission tests that mention scalar control
  flow, terminators, phi lowering, or fail-closed scalar-control-flow
  diagnostics.

## Current Scope

- CFG, terminator, and phi-lowering paths in the BIR semantic producer.
- Representative RV64 scalar-control-flow rows, including
  `src/20000314-3.c` function `attr_eq` or a current stronger substitute.
- Focused BIR evidence for semantic fact publication or explicit fail-closed
  owner-boundary rejection.

## Non-Goals

- Function-signature return-info or parameter-layout repair.
- Scalar-binop instruction repair.
- RV64 ABI/object emission repair.
- Local-memory, call metadata, runtime/intrinsic, bootstrap, or global
  data-shape work.
- Classification-only, expectation-only, unsupported-marker, or allowlist
  changes.

## Working Model

The source idea records that a previous scalar-control-flow runbook advanced
the tracked representatives beyond their original `scalar-control-flow
semantic family` diagnostics, but closure was rejected because the close gate
did not have a matching `test_after.log`. This activation should first
re-establish the current evidence rather than reopen broad implementation work
by default.

## Execution Rules

- Keep routine progress and packet proof in `todo.md`.
- Keep implementation changes narrowly tied to CFG, terminator, or phi-lowering
  semantic producer behavior.
- If a representative now fails at a downstream owner boundary, record the
  owner boundary in `todo.md`; do not expand this plan into that downstream
  work.
- If Step 1 proves the source idea is already satisfied, request a plan-owner
  close decision instead of inventing extra code work.
- For code-changing steps, run:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

## Step 1: Refresh Scalar-Control-Flow Evidence

Goal: Determine whether the scalar-control-flow source idea is already
acceptance-ready under current code and logs.

Actions:
- Inspect the current focused backend BIR tests for scalar-control-flow
  producer coverage.
- Re-run direct diagnostics for `src/20000314-3.c` function `attr_eq` or a
  current stronger scalar-control-flow substitute.
- Compare nearby same-family scalar-control-flow representatives where the
  inventory is available, especially rows that previously carried the
  `scalar-control-flow semantic family` admission diagnostic.
- Run the backend proof command and refresh `test_after.log` with the delegated
  proof output.
- Record in `todo.md` whether each representative passes, advances to a
  downstream owner boundary, or still fails at the original scalar-control-flow
  producer boundary.

Completion Check:
- `todo.md` names the refreshed representative set and current owner boundary.
- `test_after.log` contains a fresh backend proof result.
- The next packet is either Step 2 for a real scalar-control-flow producer
  repair or Step 3 for broader validation and closure handoff.

## Step 2: Repair Any Remaining Scalar-Control-Flow Producer Boundary

Goal: Fix only a confirmed remaining scalar-control-flow semantic producer
failure.

Actions:
- Work in the real CFG, terminator, or phi-lowering producer path, such as
  `BirFunctionLowerer::lower()`, `collect_phi_lowering_plans()`,
  `lower_block_phi_insts()`, `initialize_aggregate_phi_state()`,
  `apply_pending_aggregate_phi_copies()`, or `lower_block_terminator()`.
- Add or tighten focused BIR coverage for the producer behavior or fail-closed
  owner-boundary diagnostic.
- Avoid named-case shortcuts for `src/20000314-3.c`, `attr_eq`, or any single
  row shape.
- Prove the focused test and the backend subset.
- Update `todo.md` with the changed producer rule, proof command, and any
  downstream owner boundary discovered.

Completion Check:
- The original scalar-control-flow producer diagnostic is repaired or replaced
  by an explicit fail-closed owner-boundary diagnostic at the real producer.
- Focused BIR coverage demonstrates the behavior.
- Backend proof is green, and no expectation/unsupported/allowlist downgrade
  was used.

## Step 3: Broader Validation And Closure Handoff

Goal: Decide whether the source idea is complete after refreshed evidence and
any needed repair.

Actions:
- Run the supervisor-delegated broader validation command, or at minimum the
  backend subset if no broader command is delegated.
- Confirm that remaining failures, if any, are downstream owner boundaries and
  not scalar-control-flow semantic producer failures.
- Record closure evidence and residual risks in `todo.md`.
- Ask the plan owner to decide whether to close, continue, or split a new open
  idea.

Completion Check:
- `todo.md` contains enough current proof for a close decision.
- The source idea is either ready for close gate review or has a concrete
  remaining scalar-control-flow producer packet.
