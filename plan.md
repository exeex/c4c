# RV64 Terminator Fragment Lowering Runbook

Status: Active
Source Idea: ideas/open/611_rv64_terminator_fragment_lowering.md

## Purpose

Activate idea 611 as the current execution route for RV64 terminator-fragment
consumer lowering.

## Goal

Lower supported prepared terminator fragments in the RV64/MIR consumer while
preserving fail-closed behavior for missing branch operands, missing
stack-source freshness, and missing prepared control-flow authority.

## Core Rule

Implement semantic terminator-fragment consumer support only when prepared
branch/control-flow authority is explicit; do not infer missing operands from
final layout or combine this route with source-authority publication.

## Read First

- `ideas/open/611_rv64_terminator_fragment_lowering.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
- `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
- `ideas/closed/594_rv64_branch_stack_source_consumption_followup_from_593.md`

## Current Targets

- RV64 backend-object rows reporting unsupported terminator-fragment lowering.
- Prepared terminator fragments that already carry required branch operands and
  control-flow facts.
- Negative rows whose first owner is still branch stack-source freshness or
  prepared authority.

## Non-Goals

- Do not publish prepared branch source authority.
- Do not change move-bundle materialization, generic instruction fragments,
  ABI lowering, runtime behavior, expectations, unsupported markers,
  allowlists, timeout/accounting, or GCC torture classification metadata.
- Do not add named-case shortcuts for one testcase shape.

## Working Model

- Treat `unsupported_terminator_fragment` rows as RV64/MIR consumer candidates
  only after diagnostic refresh confirms explicit prepared terminator evidence.
- Split rows whose first owner is freshness, operand publication, or other
  prepared authority gaps before implementation.
- Keep object-emission support fail-closed when required terminator operands or
  control-flow facts are absent or ambiguous.

## Execution Rules

- Start each implementation packet with current diagnostics, not stale bucket
  counts.
- Prefer a narrow proof case plus nearby positive and negative rows over a
  single testcase.
- Each code-changing step must run:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- Escalate to reviewer if the diff weakens unsupported contracts, rewrites
  expectations, or proves progress only by matching a named source file.

## Step 1: Refresh Terminator-Fragment Residual Diagnostics

Goal: confirm the current unsupported terminator-fragment population and split
consumer-owned rows from authority-owned rows.

Primary target:
- RV64 backend-object logs and focused probes for the current
  `unsupported_terminator_fragment` bucket.

Actions:
- Rebuild before diagnostics if needed.
- Inspect current per-case RV64 backend logs for terminator-fragment rows.
- Run focused direct object probes for representative rows when existing logs
  are stale.
- Classify rows by first owner:
  - supported terminator-fragment consumer candidate
  - missing branch operand authority
  - missing branch stack-source freshness
  - missing prepared control-flow fact
  - downstream non-terminator owner
- Record the exact next implementation packet in `todo.md`.

Completion check:
- `todo.md` names the current row count, representative positive candidates,
  negative authority rows, and the exact proof command for Step 2.

## Step 2: Implement First Supported Terminator Consumer Rule

Goal: lower one general, explicitly authorized terminator-fragment family in
the RV64 consumer.

Primary target:
- RV64/MIR object-emission terminator lowering path selected by Step 1.

Actions:
- Locate the RV64 consumer code that rejects the chosen authorized terminator
  fragment.
- Add the smallest semantic lowering rule for the shared fragment shape.
- Require explicit prepared branch operands and control-flow facts.
- Preserve the current rejection for missing freshness or missing authority.
- Avoid changes to prepared publication, expectations, and unsupported markers.

Completion check:
- Several nearby rows from the chosen family compile or move past
  `unsupported_terminator_fragment`.
- Focused negative rows still fail closed for the original missing-authority
  reason.
- Backend subset proof passes.

## Step 3: Broaden Within Authorized Terminator Families

Goal: extend support only to adjacent terminator-fragment shapes with the same
complete prepared authority model.

Primary target:
- Additional terminator-fragment rows proven by Step 1 or Step 2 diagnostics.

Actions:
- Re-run focused residual probes after Step 2.
- Identify adjacent shapes that have complete prepared operands and
  control-flow authority.
- Add narrowly scoped consumer handling for those shapes.
- Keep missing-authority and branch stack-source rows out of scope.

Completion check:
- The remaining unsupported terminator-fragment rows are materially reduced or
  reclassified to separate first owners.
- Negative proof still covers missing operand/freshness/control-flow rows.
- Backend subset proof passes.

## Step 4: Close-Readiness Classification

Goal: decide whether idea 611 is complete or needs a regenerated runbook or
split follow-up.

Actions:
- Refresh the terminator-fragment residual set.
- Confirm whether remaining rows are outside idea 611 because they require
  prepared publication, branch stack-source freshness, ABI, instruction
  fragments, runtime, or other non-terminator owners.
- Record close readiness and any required follow-up idea in `todo.md`.

Completion check:
- `todo.md` states whether idea 611 is close-ready, not close-ready, or should
  be split.
- The recommendation is backed by current diagnostics and backend subset proof
  when code changed in the route.
