# Prepared Branch Stack-Source Freshness Publication Runbook

Status: Active
Source Idea: ideas/open/636_prepared_branch_stack_source_freshness_publication.md

## Purpose

Publish prepared branch stack-source freshness authority for branch stack-load
operands that currently have no selected freshness candidate.

## Goal

Move the idea-636 no-candidate branch stack-load rows from missing prepared
freshness authority to the next legitimate owner, or reclassify them with
current concrete evidence.

## Core Rule

Freshness must be published by prepared branch stack-source authority. Do not
infer RV64 source freshness from stack layout, payload shape, testcase names,
or target-local consumer fallback.

## Read First

- `ideas/open/636_prepared_branch_stack_source_freshness_publication.md`
- `ideas/closed/615_branch_stack_source_residual_audit.md`
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
- `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md`
- `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
- `ideas/closed/594_rv64_branch_stack_source_consumption_followup_from_593.md`
- `ideas/closed/596_pointer_rhs_branch_stack_source_policy_publication.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`

## Current Targets

- Representative rows: `src/930930-1.c`, `src/990127-1.c`, and
  `src/20060910-1.c`.
- Proof surface: rows that still stop at
  `missing_source_freshness_authority` / `no_candidate`.
- Owning layer: prepared branch stack-source freshness authority.

## Non-Goals

- Do not add clobber-safety authority for rows that already have selected
  freshness; that belongs to idea 635.
- Do not add RV64 target-local inference of source freshness.
- Do not merge select publication, compare publication, terminator lowering,
  ABI, runtime, expectation, unsupported-marker, allowlist, timeout, or
  accounting work into this plan.
- Do not claim diagnostic wording, helper renames, or expectation changes as
  capability progress.

## Working Model

- Branch stack-load consumers require explicit prepared branch-point freshness.
- The failing rows have no selected source freshness candidate:
  `authority_status=missing_source_freshness_authority`,
  `source_freshness_status=no_candidate`, and
  `source_freshness_candidates=0`.
- A valid fix should prove the source slot, branch block, terminator point,
  and value relation before publishing freshness.
- Negative states must remain precise for missing slot identity, ambiguous
  freshness, stale branch-point evidence, and unrelated select/compare/
  terminator owners.

## Execution Rules

- Start each implementation packet by refreshing current diagnostics for the
  target rows instead of assuming idea-615 evidence is still exact.
- Add producer/publication coverage before relying on RV64 consumer proof.
- Keep branch stack-source freshness publication separate from clobber-safety
  and destination fan-in authority.
- Treat testcase-shaped matching as route drift. The publication rule must be
  semantic enough to cover the supported freshness shape, not one named row.
- Use the supervisor-delegated proof command for executor packets. For
  acceptance, expect at least build proof plus focused producer/publication
  tests and a narrow row probe selected by the supervisor.

## Steps

### Step 1: Refresh No-Candidate Branch Stack-Source Evidence

Goal: confirm the current first owner and diagnostics for the idea-636 rows.

Primary target: branch stack-load source freshness diagnostics for
`src/930930-1.c`, `src/990127-1.c`, and `src/20060910-1.c`.

Actions:
- Inspect the current diagnostic path for the three representative rows.
- Confirm whether each row still reports missing source freshness authority,
  `source_freshness_status=no_candidate`, and zero candidates.
- Identify the prepared producer surface that has enough information to prove
  source slot, branch block, terminator point, and value relation.
- Record rows that are no longer in this bucket with their current first
  owner; do not silently expand this plan to those owners.

Completion check:
- `todo.md` records the current row evidence, the shared publication target,
  and any rows reclassified out of the no-candidate bucket.

### Step 2: Add Focused Producer/Publication Coverage

Goal: prove the intended branch stack-source freshness publication contract
before relying on RV64 consumer behavior.

Primary target: focused prepared/prealloc or backend tests that exercise a
legal branch stack-load freshness shape and fail-closed negative states.

Actions:
- Add or update focused tests for one legal prepared branch stack-load source
  freshness shape.
- Add negative coverage for missing source slot identity, ambiguous freshness,
  stale branch-point evidence, or unrelated owner shapes when feasible.
- Keep expectations tied to prepared freshness facts and diagnostics, not final
  assembly shape or row names.

Completion check:
- Focused tests fail before the producer change or document the existing gap,
  then pass after the publication behavior is implemented.

### Step 3: Publish Prepared Branch Stack-Source Freshness

Goal: publish explicit prepared branch stack-source freshness for proven
branch stack-load operands.

Primary target: prepared branch stack-source freshness producer path selected
in Step 1.

Actions:
- Introduce or extend the producer authority facts for
  `BranchStackLoadSource` / `BranchStackSlot` freshness.
- Require proof of source slot, branch block, terminator point, and value
  relation before publication.
- Preserve fail-closed diagnostics for absent candidates, ambiguous candidates,
  stale freshness, and missing branch-point authority.
- Avoid RV64 consumer inference or row-specific shortcuts.

Completion check:
- At least one legal no-candidate row now has selected prepared freshness and
  moves to the next legitimate owner, or current evidence proves no legal first
  publication packet exists under this source idea.

### Step 4: Validate RV64 Consumption Boundary

Goal: verify RV64 consumes only prepared authority and does not gain target
local freshness inference.

Primary target: RV64 branch stack-load consumer diagnostics for the idea-636
rows.

Actions:
- Rerun the representative rows after producer publication.
- Confirm rows that gain freshness pass the missing-authority blocker and land
  on the next real owner.
- Confirm rows without valid prepared authority remain rejected with precise
  diagnostics.
- Check nearby branch stack-load freshness cases if the producer change has
  broader reach.

Completion check:
- The proof log shows branch stack-source freshness is consumed only when the
  prepared producer published it, and rejected otherwise.

### Step 5: Acceptance Proof and Handoff

Goal: package the slice with enough evidence for supervisor acceptance.

Actions:
- Run the supervisor-selected build and narrow test/probe commands.
- Escalate to broader validation if the producer change touches shared
  freshness infrastructure beyond this branch stack-source bucket.
- Update `todo.md` with the final proof, rows moved, rows rejected, and any
  follow-up owner that should become a separate idea.

Completion check:
- `todo.md` contains fresh proof results and no open acceptance blocker for
  the idea-636 runbook.
