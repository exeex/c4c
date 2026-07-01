# Prepared Global Data And Stack Frame Infrastructure Review Runbook

Status: Active
Source Idea: ideas/open/424_prepared_global_stack_frame_infrastructure_review.md
Activated from: open idea inventory after closure of RV64 frame-slot call argument publication

## Purpose

Classify remaining prepared global-data, stack-frame, and parameter-home
failures so later implementation work starts from explicit producer facts
instead of RV64 reconstruction.

## Goal

Bucket `unsupported_global_data`, `unsupported_stack_frame`, related
`unsupported_param_home`, and nearby infrastructure failures into
producer-contract gaps versus coherent RV64 emission work, then create focused
follow-up ideas that cite the rows they own.

## Core Rule

Do not infer labels, relocations, zero-fill ranges, frame slots, offsets,
widths, or parameter homes in RV64 from raw source shape, testcase names, value
names, object labels, or final failure buckets. Missing prepared authority must
remain fail-closed and become producer-owned follow-up work.

## Read First

- `ideas/open/424_prepared_global_stack_frame_infrastructure_review.md`
- `docs/rv64_gcc_torture_post_contract/current_scan_summary.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
- `docs/rv64_gcc_torture_post_contract/followup_idea_plan.md`
- `docs/rv64_gcc_torture_post_contract/runtime_mismatch_groups.md`
- `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv`, if present
- `build/agent_state/rv64_gcc_c_torture_backend_failed.full.txt`, if present

## Current Targets And Scope

- Primary buckets: `unsupported_stack_frame`, `unsupported_global_data`, and
  `unsupported_param_home`.
- Fresh scan counts from the handoff: 84 stack-frame rows, 40 global-data rows,
  and 4 parameter-home rows.
- Owning layers: prepared contract, BIR/prepared producers, and RV64/MIR
  object emission only after prepared facts are coherent.
- Expected output: classification artifacts and separate follow-up ideas, not a
  direct implementation repair inside this runbook.

## Non-Goals

- Do not implement RV64 global-data, stack-frame, or parameter-home repairs in
  this triage route.
- Do not include F128 local memory, F128 parameter homes, or F128 ABI plumbing.
- Do not reconstruct missing object data, frame layout, save slots, offsets, or
  parameter homes inside RV64 lowering.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.
- Do not fold unrelated runtime mismatch, inline-asm, scalar FPR, or pointer
  provenance work into this infrastructure review.

## Working Model

- The current RV64 gcc_torture handoff records high-count stack-frame and
  global-data unsupported buckets after higher-priority runtime repairs.
- Some rows may already have coherent prepared object/frame facts and only need
  RV64 emission work.
- Other rows may lack stable producer authority for labels, relocations,
  zero-fill ranges, frame slots, widths, alignments, or parameter homes.
- The correct output is a row-backed split so implementation agents can choose
  the right owner without testcase-shaped inference.

## Execution Rules

- Preserve source intent in the idea file; routine evidence belongs in
  `todo.md` or generated handoff docs.
- Treat RV64 gcc_torture as external evidence, not default harness coverage.
- Classify by first owning layer, not by final unsupported diagnostic alone.
- When a row needs producer facts, create or recommend a producer-owned idea
  before any RV64 consumer route.
- When a row has coherent prepared facts, create or recommend a narrow RV64
  emission idea with focused acceptance criteria.
- For any generated follow-up idea, include reviewer reject signals that block
  testcase-shaped shortcuts and expectation downgrades.

## Step 1: Rebuild Infrastructure Bucket Evidence

Goal: establish the current row set and representative spread for global-data,
stack-frame, and parameter-home infrastructure failures.

Primary target: fresh scan artifacts and existing handoff docs.

Actions:

- Inspect the existing backend scan artifacts and handoff docs for the target
  buckets.
- If the row artifacts are missing or stale enough to block classification,
  regenerate only the focused inventory needed for these buckets.
- Record counts, representative rows, final diagnostics, and available dump
  paths in `todo.md` or a handoff artifact under
  `docs/rv64_gcc_torture_post_contract/`.
- Keep the bucket inventory separate from runtime mismatch ownership and F128
  quarantine work.

Completion check:

- The executor can name the target bucket counts, representative rows, and
  evidence files that Step 2 will classify.

## Step 2: Classify Producer Facts Versus RV64 Emission Gaps

Goal: identify which representative failures have coherent prepared facts and
which first need producer-contract work.

Primary target: representative BIR, prepared-BIR, MIR, and RV64 object-emission
evidence for the target buckets.

Actions:

- Inspect representative rows from `unsupported_stack_frame`,
  `unsupported_global_data`, and `unsupported_param_home`.
- For global-data rows, classify labels, relocations, zero-fill, selected data,
  object extents, and alignment facts as coherent, missing, or contradictory.
- For stack-frame rows, classify frame slots, save slots, object slots,
  offsets, widths, alignment, and lifetime facts as coherent, missing, or
  contradictory.
- For parameter-home rows, classify whether the ABI/prepared producer publishes
  enough home, width, and offset authority for RV64 consumption.
- Preserve fail-closed diagnostics when authority is missing or ambiguous.

Completion check:

- Each inspected representative is assigned to producer-contract work, coherent
  RV64 emission work, or intentionally parked/unsupported scope with evidence.

## Step 3: Write Row-Backed Infrastructure Handoff

Goal: turn classification evidence into durable handoff documentation without
changing source intent.

Primary target:
`docs/rv64_gcc_torture_post_contract/global_stack_frame_infrastructure_review.md`

Actions:

- Summarize bucket counts, representative rows, evidence paths, and owning
  layer decisions.
- Separate producer gaps from target-emission gaps in distinct sections.
- Call out parked F128 or out-of-scope rows explicitly instead of folding them
  into ordinary infrastructure work.
- Include reject signals for RV64 reconstruction of missing facts and
  expectation or unsupported-marker changes.

Completion check:

- The handoff doc explains which rows can feed follow-up ideas and which rows
  must remain fail-closed or parked.

## Step 4: Create Focused Follow-Up Ideas

Goal: create separate open ideas for coherent producer-contract and RV64
emission initiatives discovered by the review.

Primary target: new files under `ideas/open/` when classification supports
them.

Actions:

- Create producer-owned follow-up ideas for missing or incoherent prepared
  global-data, frame-layout, stack-slot, or parameter-home facts.
- Create RV64 emission follow-up ideas only for rows with coherent prepared
  facts and a clear target-lowering gap.
- Cite the handoff rows and evidence files each follow-up consumes.
- Include concrete reviewer reject signals in every new idea.
- Do not combine producer repair and RV64 consumption in one implementation
  idea.

Completion check:

- Follow-up ideas are source-intent documents with clear owners, acceptance
  criteria, and testcase-overfit rejection rules.

## Step 5: Validate And Handoff

Goal: leave the lifecycle state ready for closure or the next selected
implementation idea.

Primary target: handoff docs, generated ideas, and lifecycle proof.

Actions:

- Run formatting/diff checks for edited lifecycle and documentation files.
- Run default or supervisor-selected CTest proof if the route unexpectedly
  changes harness-visible files.
- Record proof commands and outcomes in `todo.md`.
- Summarize whether the source idea is complete or whether the active runbook
  needs another classification packet.

Completion check:

- Handoff docs and generated follow-up ideas are ready for supervisor review
  without implementation changes, expectation downgrades, or owner mixing.
