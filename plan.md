# Scalar FPR Salvage From try_gcc_torture Runbook

Status: Active
Source Idea: ideas/open/425_scalar_fpr_salvage_from_try_gcc_torture.md
Activated From: post-515 return to RV64 gcc_torture follow-up queue

## Purpose

Recover useful non-F128 scalar floating-point lessons from the failed
`try_gcc_torture` exploration without replaying that branch or letting
`conversion.c`/F128 work drive the next RV64 route.

## Goal

Produce a bucket-backed shortlist of scalar F32/F64 salvage candidates, reject
low-value or F128-entangled work, and create ordered follow-up ideas for the
high-impact candidates.

## Core Rule

Use current RV64 gcc_torture bucket evidence and broad ordinary-C impact to
justify salvage. Do not cherry-pick `try_gcc_torture` commits directly, do not
use `conversion.c` as the KPI, and do not bring F128 helper, ABI, or local
memory work into scalar FPR salvage.

## Read First

- ideas/open/425_scalar_fpr_salvage_from_try_gcc_torture.md
- ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md
- docs/rv64_gcc_torture_post_contract/current_scan_summary.md
- docs/rv64_gcc_torture_post_contract/failure_bucket_map.md
- docs/rv64_gcc_torture_post_contract/followup_idea_plan.md
- Reference branch: `try_gcc_torture`
- Current branch history around already-closed RV64 gcc_torture follow-ups

## Current Targets

- Candidate families:
  - scalar F32/F64 casts
  - scalar FPR binary operations
  - scalar floating select/materialization
  - FPR local-store/reload
  - floating branch guard handling
- Evidence sources:
  - current RV64 gcc_torture bucket counts
  - `try_gcc_torture` commit history and diffs
  - existing handoff docs under `docs/rv64_gcc_torture_post_contract/`
- Required output:
  - a recorded salvage shortlist with commit references, bucket impact, and
    rewrite risk
  - explicit quarantine or rejection notes for low-value or F128-entangled
    candidates
  - ordered follow-up ideas for high-frequency non-F128 scalar/FPR work

## Non-Goals

- Do not cherry-pick `try_gcc_torture` wholesale.
- Do not implement scalar FPR lowering inside this planning runbook.
- Do not include F128 helpers, F128 ABI, F128 local-memory, or
  `conversion.c`-driven work.
- Do not broaden into unrelated RV64 object-emission rewrites without current
  bucket evidence.
- Do not alter expectations, allowlists, unsupported markers, runtime
  comparison, scan accounting, or default CTest contracts.

## Working Model

- Idea 420 established that `try_gcc_torture` found useful implementation
  facts but lost broad RV64 gcc_torture coverage because it over-prioritized
  `conversion.c` and F128.
- Later RV64 follow-ups consumed the dominant move-bundle queue and closed the
  immediate prepared move-bundle successors through idea 515.
- Scalar FPR salvage is valid only where current bucket evidence shows broad
  non-F128 value and where the rewrite can be separated from F128 or
  conversion-only artifacts.
- This runbook should create implementation-ready ideas, not perform the
  implementation.

## Execution Rules

- Keep review artifacts in `todo.md` and any executor-owned docs or source
  ideas directed by the supervisor; do not mutate the parent umbrella unless
  durable source intent changes.
- Treat each `try_gcc_torture` commit as evidence, not as a patch to replay.
- Cross-check every candidate against current bucket counts before ranking it.
- If a candidate requires F128, helper ABI, or `conversion.c`-only proof,
  quarantine or reject it.
- If follow-up source ideas are created, include concrete reviewer reject
  signals that block testcase-shaped scalar/FPR shortcuts and expectation
  rewrites.
- For lifecycle close, record the shortlist, rejected/quarantined candidates,
  follow-up idea order, and validation proof in `todo.md`.

## Step 1: Rebuild Scalar FPR Evidence Sources

Goal: Identify the current scalar FPR bucket evidence and the relevant
`try_gcc_torture` commit range to review.

Actions:

- Inspect current RV64 gcc_torture handoff docs and any available build
  artifacts for scalar F32/F64 failure rows.
- Identify `try_gcc_torture` commits touching scalar F32/F64 casts, scalar FPR
  binary ops, floating select/materialization, FPR local-store/reload, and
  branch guard handling.
- Separate non-F128 scalar/FPR candidates from F128, helper ABI, local-memory,
  and `conversion.c`-only work.
- Record the evidence source paths, commit hashes, and initial candidate list
  in `todo.md`.

Completion check:

- `todo.md` records the candidate commit set, bucket evidence source, and
  initial non-F128 versus F128-entangled separation.

## Step 2: Classify Candidate Impact And Rewrite Risk

Goal: Rank scalar FPR candidates by broad non-F128 bucket impact and rewrite
risk.

Actions:

- For each candidate, record expected bucket impact, representative rows if
  available, required prepared facts, and implementation surface.
- Identify whether the candidate is a clean rewrite, a partial salvage, a
  producer prerequisite, or a rejection/quarantine.
- Reject candidates justified only by `conversion.c` or by narrow named rows.
- Keep scalar FPR work separate from F128 helper, ABI, and local-memory routes.

Completion check:

- `todo.md` contains a ranked candidate table with commit references, impact,
  risk, and accept/reject/quarantine decision for each candidate.

## Step 3: Define Follow-Up Idea Boundaries

Goal: Convert the high-value salvage candidates into small follow-up idea
boundaries.

Actions:

- Group accepted candidates into implementation ideas with one owning layer
  each.
- Define in-scope and out-of-scope work for each follow-up.
- Define focused tests, representative proof, and default CTest expectations
  for each future salvaged slice.
- Write reviewer reject signals for testcase-shaped fixes,
  `conversion.c`-only proof, F128 contamination, broad rewrites, and
  expectation downgrades.

Completion check:

- `todo.md` records the proposed follow-up boundaries and reviewer reject
  signals before any source idea files are created.

## Step 4: Publish Salvage Handoff And Follow-Up Ideas

Goal: Record the durable salvage output and create ordered follow-up ideas for
the accepted non-F128 scalar/FPR candidates.

Actions:

- Publish the shortlist, rejected/quarantined candidates, and ordering in the
  appropriate handoff location selected by the supervisor.
- Create or update follow-up ideas under `ideas/open/` only for accepted
  candidates, keeping each idea small and bucket-backed.
- Record why low-value or F128-entangled candidates are not being replayed.
- Keep the active source idea unchanged unless durable intent needs repair.

Completion check:

- The salvage shortlist is recorded with commit references, expected bucket
  impact, and rewrite risk.
- Low-value and F128-entangled candidates are explicitly rejected or
  quarantined.
- Ordered follow-up ideas exist for the accepted scalar/FPR salvage work.

## Step 5: Validate And Handoff

Goal: Prove the planning slice is stable enough for closure or identify the
exact remaining salvage work.

Actions:

- Run formatting or diff checks for changed docs and idea files.
- Run default CTest if the supervisor treats this planning slice as
  close-ready or if any non-doc lifecycle change requires it.
- Update `todo.md` with proof, residual risks, and whether source idea
  acceptance criteria are satisfied.

Completion check:

- Fresh proof is recorded in `todo.md`.
- The plan owner can decide closure, or the remaining work is explicitly
  separated from scalar FPR salvage planning.
