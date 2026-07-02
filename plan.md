# Prepared Global-Data And Stack-Frame Infrastructure Review Runbook

Status: Active
Source Idea: ideas/open/548_prepared_global_stack_frame_infrastructure_review.md

## Purpose

Classify the current global-data, stack-frame, and prepared move-bundle
infrastructure failures into first-owner buckets before any implementation
work uses them as proof scope.

## Goal

Produce current, row-backed follow-up ownership for prepared-contract work,
RV64 infrastructure work, and evidence gaps, without fabricating missing
prepared facts in RV64 lowering or treating classification as capability
repair.

## Core Rule

Keep this as an infrastructure boundary review. Do not implement global-data,
stack-frame, or move-bundle lowering in this plan, and do not change
expectations, unsupported markers, allowlists, or pass/fail accounting to make
rows look advanced.

## Read First

- `ideas/open/548_prepared_global_stack_frame_infrastructure_review.md`
- Current gcc_torture backend logs or freshly reproduced representative logs
- Prepared/BIR handoff surfaces under `src/backend/prepared/` and
  `src/backend/bir/`
- RV64 object-lowering diagnostics under `src/backend/mir/riscv/`
- Existing bucket evidence for `unsupported_stack_frame`,
  `unsupported_global_data`, and
  `unsupported_prepared_move_bundle_classification`

## Scope

- Reconstruct representative current rows for the three infrastructure
  buckets named by the source idea.
- Classify each inspected row by first owner: prepared contract, RV64
  infrastructure lowering, F128 quarantine, or evidence gap.
- Split coherent producer-owned and RV64-owned follow-up ideas only after the
  current row evidence supports them.
- Preserve the distinction between prepared facts and RV64 consumption of
  those facts.

## Non-Goals

- Do not implement prepared global-data layout, RV64 global-data object
  lowering, stack-frame lowering, or move-bundle classification in this
  review plan.
- Do not fold F128-primary rows into ordinary-C infrastructure work.
- Do not replay old helper cleanup ideas as current capability progress.
- Do not merge prepared-contract repair and RV64 object lowering into one
  implementation slice.
- Do not reopen closed BIR local-memory, call-metadata,
  runtime/intrinsic-memory, or bootstrap global data-shape handoff ideas.

## Working Model

Recent producer-admission and BIR handoff work has moved several
representatives to downstream `unsupported_global_data`. This review should
start from current evidence, then decide whether the remaining failures need
prepared facts, RV64 object-route support, stack-frame infrastructure,
move-bundle authority, F128 quarantine, or more reproduction before an
implementation idea is safe.

## Execution Rules

- Start from fresh representative logs or a traceable current scan; do not
  rely on stale row counts alone.
- Keep review packets evidence-only unless the supervisor activates a
  follow-up implementation idea later.
- Record exact rows, commands, diagnostics, and likely owner boundaries in
  `todo.md`.
- If a bucket proves implementation-ready, create or request a separate
  `ideas/open/*.md` follow-up with concrete acceptance criteria and reviewer
  reject signals.
- If producer facts are missing, do not route the row to RV64 inference work.
- If a row is primarily F128, route it to the F128 quarantine policy lane.

## Steps

### Step 1: Reconstruct Infrastructure Bucket Evidence

Goal: Rebuild a current representative evidence set for the three
infrastructure buckets.

Primary targets:

- Current `unsupported_global_data` representatives, including rows recently
  handed off by runtime/intrinsic or global data-shape work when still
  relevant
- Current `unsupported_stack_frame` representatives
- Current `unsupported_prepared_move_bundle_classification` representatives

Actions:

- Produce or locate a current scan/log source for the three bucket names.
- Select a small representative allowlist that covers each bucket without
  overfitting to one filename.
- Reproduce those rows and capture the first diagnostic and immediate owning
  layer evidence.
- Screen out primary-F128 rows before ordinary-C ownership classification.

Completion check:

- `todo.md` records representative rows, log paths, counts or traceable
  evidence source, and whether each bucket has enough evidence for focused
  classification in later steps.

### Step 2: Classify Global-Data Ownership

Goal: Separate prepared global-data contract gaps from RV64 global-data
object-route consumption gaps.

Actions:

- Inspect representative `unsupported_global_data` rows from Step 1.
- Identify whether BIR/prepared symbol, data-shape, initializer, or relocation
  facts are missing.
- Identify rows where prepared facts are present but RV64 object emission or
  address materialization is missing.
- Record any F128-primary global-data rows for quarantine rather than
  ordinary-C work.

Completion check:

- `todo.md` records global-data sub-buckets, first owners, proof logs, and
  whether one or more follow-up ideas should be created.

### Step 3: Classify Stack-Frame Ownership

Goal: Separate prepared frame-fact gaps from RV64 stack-frame lowering gaps.

Actions:

- Inspect representative `unsupported_stack_frame` rows from Step 1.
- Determine whether frame layout, stack slots, spills, argument homes, or
  callee-saved facts are absent before RV64 lowering.
- Determine whether facts are present but RV64 frame setup, access, or
  teardown support is missing.
- Keep F128-driven frame needs out of the ordinary-C route.

Completion check:

- `todo.md` records stack-frame sub-buckets, first owners, proof logs, and
  whether one or more follow-up ideas should be created.

### Step 4: Classify Prepared Move-Bundle Ownership

Goal: Determine whether move-bundle classification failures need prepared
authority repair, RV64 lowering support, or fail-closed retention.

Actions:

- Inspect representative
  `unsupported_prepared_move_bundle_classification` rows from Step 1.
- Identify missing authority, unsupported source/destination forms, or
  expected fail-closed cases.
- Keep helper renames and diagnostic wording out of progress claims.

Completion check:

- `todo.md` records move-bundle sub-buckets, first owners, proof logs, and
  whether one or more follow-up ideas should be created.

### Step 5: Split Follow-Up Infrastructure Ideas

Goal: Convert the classified evidence into durable, implementation-ready
source ideas or explicitly leave evidence gaps open.

Actions:

- Create focused follow-up ideas under `ideas/open/` only for coherent,
  implementation-ready buckets.
- Include concrete rows, proof commands, acceptance criteria, and reviewer
  reject signals in each follow-up idea.
- Record unresolved evidence gaps without expanding this review into
  implementation.
- Recommend close, split, or route reset for this review idea.

Completion check:

- The review has a current ownership map for all three infrastructure buckets,
  implementation-ready follow-up ideas exist where justified, and `todo.md`
  recommends the lifecycle decision for the active source idea.
