# Branch Stack-Source Residual Audit And Repair

Status: Active
Source Idea: ideas/open/615_branch_stack_source_residual_audit.md

## Purpose

Audit the residual branch stack-source failures left after the closed branch
authority series, then repair only the single owner proven by that audit.

Goal: convert residual branch stack-load authority/source freshness evidence
into either one narrow semantic repair or durable reclassification.

Core Rule: audit first; do not make branch, terminator, expectation, or marker
changes until the residual owner is proven from current diagnostics.

## Read First

- `ideas/open/615_branch_stack_source_residual_audit.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- Closed reference ideas named by the source idea:
  - `ideas/closed/590_branch_stack_load_freshness_contract.md`
  - `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md`
  - `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
  - `ideas/closed/594_rv64_branch_stack_source_consumption_followup_from_593.md`
  - `ideas/closed/596_pointer_rhs_branch_stack_source_policy_publication.md`

## Current Scope

- Residual branch stack-load authority rows.
- Residual branch stack-load source freshness rows.
- The split-in `src/921124-1.c` stack-source portion only after proving that
  its first owner is branch stack-source freshness rather than select
  publication.

## Non-Goals

- Generic terminator lowering.
- New branch architecture not supported by the audit.
- Select publication repair unless the audit proves this idea is not the owner.
- ABI, runtime, expectations, unsupported markers, allowlists, timeout, or
  accounting changes.
- Named-case-only shortcuts.

## Working Model

The closed branch stack-source series already established several publication
and RV64 consumption contracts. Treat those contracts as authority boundaries:
the next packet must classify whether current residual rows are missing
prepared authority, missing RV64 consumption of existing authority, or outside
this idea.

## Execution Rules

- Keep routine packet findings in `todo.md`.
- If the audit finds multiple unrelated owners, stop and split or classify
  before implementation.
- If a row remains on the same old failure mode after a helper rename, reject
  the route as non-progress.
- Use backend proof for any code-changing packet:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- Keep focused object-emission or frontend probes as supporting evidence, not
  as a substitute for the delegated proof command when implementation changes.

## Step 1: Refresh Branch Stack-Source Residuals

Goal: record current residual evidence before implementation.

Actions:
- Build the current backend if needed for diagnostics.
- Refresh the branch stack-load authority/source freshness rows named by the
  current failure map.
- Compare each row against the closed contracts from ideas `590`, `592`,
  `593`, `594`, and `596`.
- Record row counts, representative testcase names, current first owner, and
  whether each row has complete prepared/RV64 authority for this idea.
- Specifically classify `src/921124-1.c` as branch stack-source freshness,
  select publication, or another owner before any code changes.

Completion Check:
- `todo.md` records the audit result and identifies either one in-scope owner
  for repair or a concrete split/classification decision.

## Step 2: Repair One Proven Branch Stack-Source Owner

Goal: implement only the single audited owner if Step 1 proves one.

Actions:
- Touch only the owner surface proven by Step 1.
- Add or update focused tests that exercise the semantic authority path, not a
  named testcase shortcut.
- Keep existing branch stack-source guards fail-closed for missing or
  contradictory authority.
- Do not mix terminator lowering, select publication, ABI, or runtime changes
  into this packet.

Completion Check:
- Multiple residual rows either move past the audited branch stack-source owner
  or are reclassified with concrete evidence.
- Existing branch stack-source guard rows stay on their original owners.
- Backend proof passes with the delegated `^backend_` command.

## Step 3: Residual Refresh And Split Decision

Goal: determine whether more in-scope branch stack-source work remains.

Actions:
- Refresh the residual branch stack-source rows after Step 2 or after the
  Step 1 no-implementation decision.
- Classify remaining failures as in-scope same-owner, separate owner, or
  downstream non-branch-stack-source work.
- If remaining work is separate, create or request durable follow-up ideas
  rather than expanding this runbook.

Completion Check:
- `todo.md` records the remaining row families, exclusions, and recommended
  next lifecycle action.

## Step 4: Close-Readiness Classification

Goal: decide whether idea `615` is complete, blocked, or needs a runbook
rewrite.

Actions:
- Verify the source idea's audit-first acceptance criteria are satisfied.
- Confirm no complete-authority branch stack-source consumer family remains
  unhandled in this idea.
- Prepare lifecycle notes for closure or split if residual work belongs
  elsewhere.

Completion Check:
- Plan owner can decide close, split, or rewrite from `todo.md` without
  re-running the whole audit.
