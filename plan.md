# AArch64 Instruction Dispatch Internal Plan

Status: Active
Source Idea: ideas/open/665_aarch64_instruction_dispatch_internal.md

## Purpose

Repair the AArch64-specific instruction dispatch and prepared-BIR publication
failures without mixing them into RV64 runtime, RISC-V object emission, or
generic prepared CLI work.

## Goal

Name the first AArch64 owner for rows 284 and 322, then repair one general
AArch64 dispatch or publication rule with focused proof and no expectation
churn.

## Core Rule

Do not treat AArch64 failures as RV64 lowering work, RISC-V object-emission
work, or generic CLI formatting unless refreshed evidence proves that route.
No testcase-shaped dispatch-table shortcuts, unsupported-marker changes,
allowlist edits, timeout changes, or baseline accounting changes count as
progress.

## Read First

- `ideas/open/665_aarch64_instruction_dispatch_internal.md`
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
- Nearby completed prepared/CLI and RV64 object-emission ideas only as
  boundary references, not as implementation scope.

## Current Targets

- `backend_aarch64_instruction_dispatch`
- `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`

## Non-Goals

- Do not repair RV64 runtime lowering, RISC-V object emission, stack fan-in
  authority, byval call-boundary behavior, object-data static storage, LLVM
  torture rows, or prepared dump rows outside the AArch64 target surface.
- Do not edit expectations, unsupported markers, allowlists, timeouts, runtime
  policy, or baseline accounting as a substitute for AArch64 capability.
- Do not classify row 322 as generic CLI text formatting while AArch64
  dispatch or publication facts are missing or stale.
- Do not broaden this plan to unrelated AArch64 work without a lifecycle split.

## Working Model

Row 284 is the primary AArch64 instruction-dispatch failure. Row 322 names
prepared-BIR AArch64 publication and may either share the AArch64 owner or
return to a generic prepared contract route if current evidence proves the
AArch64 facts already exist and only exposure is stale. The first step must
separate those possibilities before implementation.

## Execution Rules

- Refresh focused evidence before editing implementation.
- Keep row 284 and row 322 classified separately until a shared owner is
  proven.
- Prefer existing AArch64 dispatch/publication helpers and prepared fact
  surfaces over parallel special paths.
- Preserve fail-closed diagnostics for unsupported, missing, ambiguous, or
  mismatched AArch64 instruction/publication shapes.
- For code-changing packets, run the supervisor-selected focused subset and a
  nearby backend guard before claiming acceptance readiness.

## Ordered Steps

### Step 1: Refresh AArch64 Dispatch And Publication Evidence

Goal: reproduce rows 284 and 322 and identify their first observable failure
boundaries.

Actions:

- Run the focused AArch64 subset selected by the supervisor.
- Capture row 284 diagnostics, prepared input shape, and the first unsupported
  or missing dispatch contract.
- Capture row 322 prepared-BIR output and decide whether missing AArch64
  publication facts or dump/CLI exposure is the first owner.
- Record whether the two rows share an AArch64 owner or must split.

Completion check:

- `todo.md` names the first owner for row 284 and row 322, or recommends a
  lifecycle split for row 322 with concrete evidence.

### Step 2: Select The AArch64 Internal Boundary

Goal: choose the smallest general AArch64 target rule to repair first.

Actions:

- Inspect AArch64 instruction selection, dispatch-table coverage, prepared-BIR
  AArch64 publication, and CLI exposure surfaces named by Step 1 evidence.
- Select exactly one implementation boundary for the first code packet.
- Record malformed or unsupported sibling shapes that must remain fail-closed.
- If row 322 is not AArch64-owned, ask the plan owner to split or reroute it
  instead of silently absorbing it.

Completion check:

- `todo.md` records the selected owner, proof command, guard rows, and
  fail-closed watchouts for the implementation packet.

### Step 3: Repair The Selected AArch64 Rule

Goal: implement the focused AArch64 dispatch or publication repair.

Actions:

- Modify only the AArch64 target-internal or prepared-publication surfaces
  proven by the selected boundary.
- Use existing prepared facts and target helpers where they are authoritative.
- Keep unrelated target backends and generic CLI formatting untouched unless
  Step 1 proved they are the owner.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime policy, or baseline files.

Completion check:

- The supervisor-selected focused proof shows improvement for the selected
  row or fails closed with a more precise owner diagnostic.
- Nearby AArch64/CLI guard rows do not regress.
- `todo.md` records the before/after delta and any remaining first failure.

### Step 4: Prove Regression Safety And Lifecycle Readiness

Goal: decide whether idea 665 is complete, needs a split, or needs another
  narrow packet.

Actions:

- Run the focused AArch64 proof and any broader backend subset selected by the
  supervisor.
- Compare against matching before/after logs for the chosen close scope.
- If a different owner remains for row 322 or a new failure appears, split it
  into a separate open idea instead of expanding this plan.

Completion check:

- Rows owned by this idea pass or fail closed at a precise AArch64 owner.
- Regression proof has no new failures for the chosen scope.
- The plan owner can close, split, or leave the source idea active with a
  concrete next packet.
