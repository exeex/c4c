# RV64 Instruction Fragment Bucket Classification Runbook

Status: Active
Source Idea: ideas/open/421_rv64_instruction_fragment_bucket_classification.md

## Purpose

Split the broad `unsupported_instruction_fragment` RV64 gcc_torture failure
family into concrete, high-impact lowering owners without making F128 the
primary route.

## Goal

Produce bucket-backed follow-up implementation ideas for ordinary non-F128 RV64
instruction lowering families, with counts, representative rows, owning layers,
and clear reject signals.

## Core Rule

Classify by semantic owner and bucket evidence. Do not implement lowering,
weaken expectations, or route missing BIR/prepared producer facts through RV64
inference.

## Read First

- `ideas/open/421_rv64_instruction_fragment_bucket_classification.md`
- `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
- `docs/rv64_gcc_torture_post_contract/`, especially any current scan,
  failure bucket, postmortem, and follow-up plan artifacts that already exist

## Current Targets And Scope

- Target bucket: `unsupported_instruction_fragment`.
- Primary owners to identify: integer operations, scalar comparisons, shifts,
  select/materialization, call-adjacent moves, pointer arithmetic, and scalar
  F32/F64 operations.
- Required output surface: handoff docs under
  `docs/rv64_gcc_torture_post_contract/` plus follow-up ideas under
  `ideas/open/` when the evidence supports implementation work.
- Validation: lifecycle/docs slices should pass `git diff --check`; any command
  that reruns or inspects RV64 gcc_torture evidence should preserve default
  CTest stability expectations for later close.

## Non-Goals

- Do not implement RV64 lowering in this runbook.
- Do not continue `conversion.c` or F128 as the main success metric.
- Do not repair BIR or prepared producer gaps inside RV64 classification.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.
- Do not classify missing producer facts as RV64 lowering work.

## Working Model

- Treat `unsupported_instruction_fragment` as a broad umbrella bucket that must
  be decomposed before implementation.
- Separate ordinary high-frequency non-F128 work from F128 rows.
- When a row lacks coherent BIR/prepared facts, mark the owning layer as the
  producer layer and route it to a separate producer idea instead of proposing
  RV64 inference.
- Use representative rows to explain each bucket, but do not let one named row
  define the route.

## Execution Rules

- Keep routine progress in `todo.md`; update this plan only if the route or
  proof contract changes.
- Prefer existing scan artifacts before rerunning expensive external evidence.
- If fresh scan evidence is required, record the command, source branch/state,
  and artifact path in the handoff docs.
- Every proposed follow-up idea must include concrete reviewer reject signals,
  including testcase-shaped shortcuts, expectation downgrades, and producer-gap
  bypasses.
- F128 may appear only as quarantine or low-priority classification unless the
  evidence proves it blocks a broader non-F128 owner.

## Steps

### Step 1: Confirm Evidence Source And Bucket Inputs

Goal: identify the current source of truth for `unsupported_instruction_fragment`
rows on reset `main`.

Primary target: `docs/rv64_gcc_torture_post_contract/`

Actions:

- Inspect existing handoff docs for current scan summaries, failure bucket maps,
  postmortem notes, and follow-up ordering.
- Locate the artifact or command that records `unsupported_instruction_fragment`
  rows and counts.
- If no usable row-level artifact exists, define the narrow command or script
  needed to regenerate just enough evidence for classification.
- Record the chosen evidence source and any stale or missing artifacts in the
  handoff docs.

Completion check:

- The executor can name the exact artifact or command that provides the bucket
  rows, and `git diff --check` passes for any docs-only changes.

### Step 2: Classify Instruction Fragments By First Owner

Goal: split the broad bucket into concrete semantic owners with counts and
representative rows.

Primary target: `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`

Actions:

- Group rows by BIR opcode, operand/result type, prepared fact completeness, and
  likely RV64 lowering owner.
- Separate non-F128 instruction lowering rows from F128 rows.
- Mark rows that are really BIR/prepared producer gaps as producer-owned, with
  enough evidence to prevent RV64-side inference.
- Preserve representative rows for each sub-bucket.

Completion check:

- The failure bucket map contains concrete sub-buckets, counts, representative
  rows, and owning-layer classifications for the broad instruction-fragment
  family.

### Step 3: Rank High-Impact Non-F128 Follow-Ups

Goal: turn the classification into an ordered implementation backlog.

Primary target: `docs/rv64_gcc_torture_post_contract/followup_idea_plan.md`

Actions:

- Rank ordinary non-F128 buckets before F128 work.
- Prefer broad semantic owners over testcase-shaped or file-name-driven routes.
- Identify at least three high-impact non-F128 follow-up implementation ideas if
  the evidence supports them.
- Explicitly mark F128 as quarantine or lowest priority unless it blocks a
  broader non-F128 owner.

Completion check:

- The follow-up plan orders implementation candidates by bucket evidence,
  expected broad impact, and owning layer, and explains why F128 is not the
  primary route.

### Step 4: Write Follow-Up Ideas For Implementation-Ready Buckets

Goal: create durable open ideas for bucket-backed implementation work.

Primary target: `ideas/open/`

Actions:

- Create one small implementation idea per coherent high-impact owner.
- Keep producer repair and RV64 lowering in separate ideas.
- Include goal, why, in-scope work, out-of-scope work, acceptance criteria, and
  concrete reviewer reject signals in each new idea.
- Cite the handoff rows or bucket docs that justify each idea.

Completion check:

- Each new idea is implementation-ready, cites its evidence, names its owning
  layer, and has reviewer reject signals strong enough to block overfit routes.

### Step 5: Validate And Prepare Lifecycle Close Decision

Goal: prove the classification slice is stable and ready for supervisor review.

Primary target: lifecycle state and touched docs/idea files

Actions:

- Run `git diff --check`.
- Run the supervisor-delegated proof command for this lifecycle/docs slice; if
  no code changed, do not invent a broad code validation requirement.
- Summarize completed buckets, generated ideas, postponed F128 handling, and any
  unresolved evidence gaps in `todo.md`.

Completion check:

- Proof results are recorded in `todo.md`, touched files are ready for review,
  and the supervisor can decide whether the source idea is complete or needs a
  follow-up runbook.
