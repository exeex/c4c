# RV64 Move-Bundle Target-Shape Bucket Split Runbook

Status: Active
Source Idea: ideas/open/544_rv64_move_bundle_target_shape_bucket_split.md

## Purpose

Turn the stable 2026-07-02 `unsupported_move_bundle_target_shape` bucket into
auditable follow-up work. The active source idea is a review and splitter
route, not an implementation route.

## Goal

Reconstruct the 183 current rows, classify each row by first owner, and record
coherent follow-up queues without changing compiler behavior or test
contracts.

## Core Rule

Do not infer missing target-shape authority from testcase names, register
spellings, raw BIR fragments, or expected RV64 destination shapes. A row becomes
RV64 implementation work only when prepared facts are complete enough for a
reviewer to audit that ownership.

## Read First

- `ideas/open/544_rv64_move_bundle_target_shape_bucket_split.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
- `docs/rv64_gcc_torture_post_contract/current_scan_summary.md`
- `docs/rv64_gcc_torture_post_contract/followup_idea_plan.md`

## Current Targets

- Stable 2026-07-02 `unsupported_move_bundle_target_shape` rows.
- The 183-row current bucket named by the source idea.
- Representative current per-case evidence and scan summaries needed to make
  row ownership auditable.

## Non-Goals

- Do not implement RV64 lowering in this plan.
- Do not repair prepared-module, BIR, runtime, or F128 behavior in this plan.
- Do not change gcc_torture expectations, unsupported markers, allowlists, or
  runtime comparison behavior.
- Do not treat F128-primary rows as ordinary-C move-bundle progress.
- Do not collapse producer repair and RV64 materialization into one follow-up
  idea.

## Working Model

Classify each row into one first-owner lane:

- coherent RV64/MIR materialization
- prepared-module target-shape authority
- BIR semantic producer
- F128 quarantine
- evidence gap

Rows can remain in an evidence-gap lane when current artifacts are not strong
enough to assign ownership. That is better than guessing and producing
testcase-overfit implementation work.

## Execution Rules

- Preserve the 183-row total and explain any mismatch immediately in
  `todo.md`.
- Keep row-level evidence close to the classification result so a reviewer can
  audit each route decision.
- Create or update durable follow-up ideas only after classification produces
  coherent implementation queues.
- Keep routine packet progress in `todo.md`; edit this runbook only if the
  route contract changes.
- Use docs-only proof for classification artifacts. If an executor touches
  implementation code despite this plan, stop and route back to the supervisor.

## Ordered Steps

### Step 1: Reconstruct The Bucket

Goal: rebuild the current 183-row `unsupported_move_bundle_target_shape`
working set from stable scan evidence.

Primary targets:

- 2026-07-02 scan artifacts referenced by the RV64 post-contract docs
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`

Actions:

- Find the exact artifact or command output that names the current bucket rows.
- Record the artifact path, scan date, total row count, and any filtering rule
  used to isolate ordinary-C rows.
- Preserve enough row identifiers for later packets to classify without
  re-deriving the bucket.
- If the bucket cannot be reconstructed to 183 rows, stop and record the
  mismatch in `todo.md` before further classification.

Completion check:

- `todo.md` names the evidence source and confirms either a reproducible
  183-row bucket or a concrete blocker.

### Step 2: Define Classification Evidence

Goal: establish the minimum facts required to assign first-owner lanes.

Primary targets:

- current per-case logs for representative rows
- prepared/BIR/RV64 diagnostics visible in the stable scan evidence

Actions:

- Define what evidence proves a row is coherent RV64/MIR materialization work.
- Define what evidence proves prepared-module target-shape authority is missing
  or inconsistent.
- Define what evidence proves a BIR semantic producer must be repaired before
  RV64 lowering.
- Define F128-primary and evidence-gap routing rules.
- Record the rules where the row classification will be stored.

Completion check:

- The classification artifact has explicit lane definitions that match the
  source idea reject signals.

### Step 3: Classify Representative Rows

Goal: test the classification rules on a small, auditable subset before
processing the full bucket.

Primary targets:

- rows with explicit `unsupported_move_bundle_target_shape` diagnostics
- rows with visible prepared/BIR authority clues
- rows suspected to be F128-primary

Actions:

- Select representative rows from each visible diagnostic shape.
- Attach current evidence snippets or artifact references for each selected
  row.
- Classify each selected row into exactly one first-owner lane, or mark it as
  an evidence gap.
- Reject any classification that depends only on testcase names or expected
  target register spelling.

Completion check:

- Representative rows cover the observed diagnostic shapes and reveal no rule
  that would force testcase-shaped ownership.

### Step 4: Classify The Full Bucket

Goal: assign every reconstructed row to a first-owner lane.

Primary targets:

- the full reconstructed 183-row bucket
- the classification artifact chosen in Step 2

Actions:

- Apply the Step 2 rules to all bucket rows.
- Preserve row identifiers, first-owner lane, and evidence reference for each
  row.
- Keep uncertain rows in the evidence-gap lane instead of guessing ownership.
- Verify the final lane counts sum to the reconstructed bucket total.

Completion check:

- Every row has one lane, the counts reconcile to 183, and the artifact is
  reviewer-auditable.

### Step 5: Split Follow-Up Queues

Goal: turn coherent lanes into separate durable implementation or review ideas.

Primary targets:

- coherent RV64/MIR materialization rows
- prepared-module authority gaps
- BIR semantic producer gaps
- evidence-gap and F128 quarantine rows

Actions:

- Create separate follow-up ideas only for lanes whose evidence is coherent
  enough to own implementation.
- Keep producer repair and RV64 materialization in different ideas.
- Route F128-primary rows to the existing F128 quarantine lane instead of
  creating ordinary-C work.
- Leave evidence-gap rows as a review queue with the missing evidence stated.

Completion check:

- Follow-up ideas or documented subqueues map cleanly to classified lanes, and
  no implementation route mixes first-owner responsibilities.

### Step 6: Close-Readiness Review

Goal: decide whether the source idea can close without implementation changes.

Primary targets:

- classification artifact
- generated follow-up ideas or subqueues
- `todo.md` proof records

Actions:

- Confirm the 183-row bucket is reproducible or the blocker is explicit.
- Confirm every row is classified or consciously left in an evidence-gap queue.
- Confirm coherent RV64, prepared, BIR, F128, and evidence-gap work is routed
  separately.
- Confirm there were no implementation, expectation, unsupported-marker,
  allowlist, or runtime-comparison changes.

Completion check:

- The supervisor has enough evidence to ask the plan owner to close, deactivate,
  or repair the runbook.
