# RV64 gcc_torture Post-Contract Umbrella Runbook

Status: Active
Source Idea: ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md

## Purpose

Transcribe the post-contract RV64 gcc_torture umbrella into an execution
runbook for evidence collection, route review, failure ownership, and follow-up
idea generation.

## Goal

Use reset `main` plus the preserved `try_gcc_torture` branch as evidence to
produce an ordered RV64 recovery plan that prioritizes broad ordinary-C
coverage and keeps F128 in a quarantine or low-priority policy lane.

## Core Rule

Do not implement RV64 fixes in this umbrella. Produce traceable docs and
follow-up ideas only. RV64 gcc_torture is external evidence, not a default CTest
gate, and F128 must not become the main progress route.

## Read First

- `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
- `ideas/open/426_f128_quarantine_and_external_softfloat_policy.md`
- `docs/rv64_gcc_torture_post_contract/README.md`
- `docs/rv64_gcc_torture_post_contract/current_scan_summary.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
- `docs/rv64_gcc_torture_post_contract/followup_idea_plan.md`

## Current Targets

- `docs/rv64_gcc_torture_post_contract/current_scan_summary.md`
- `docs/rv64_gcc_torture_post_contract/regression_delta.md`
- `docs/rv64_gcc_torture_post_contract/try_gcc_torture_postmortem.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
- `docs/rv64_gcc_torture_post_contract/followup_idea_plan.md`
- new follow-up ideas under `ideas/open/`

## Non-Goals

- Do not replay or cherry-pick the `try_gcc_torture` branch as part of this
  umbrella.
- Do not continue `conversion.c` or F128 as the primary KPI.
- Do not repair BIR, prepared, MIR, RV64, runtime, or test infrastructure
  capability in this plan.
- Do not add RV64 gcc_torture to the default harness.
- Do not weaken unsupported markers, allowlists, expected output, runtime
  comparison, pass/fail accounting, or default CTest contracts.
- Do not mix BIR producer repair and MIR/RV64 lowering in one generated
  implementation idea.

## Working Model

- The reset-main post-contract baseline and the `try_gcc_torture` branch are
  evidence sources for route planning, not implementation patches to accept
  wholesale.
- Failure ownership must identify the first responsible layer before any
  implementation idea is selected.
- Follow-up ideas should rank broad ordinary-C value ahead of niche F128 work.
- Primary-F128 rows should be screened into the existing F128 quarantine lane
  unless fresh evidence shows they block broad non-F128 progress.
- Any BIR or prepared producer gap discovered by this umbrella should become a
  producer-owned idea before dependent MIR/RV64 lowering is attempted.

## Execution Rules

- Keep source-idea intent stable; record routine packet progress in `todo.md`.
- Prefer row-level or bucket-level evidence over anecdotal testcase examples.
- When creating follow-up ideas, include owning layer, acceptance criteria, and
  concrete reviewer reject signals.
- Treat docs and generated idea files as the deliverables for this runbook.
- For docs-only or idea-only steps, use targeted `git diff --check` proof.
- Before lifecycle close, run default CTest non-regression proof unless the
  supervisor supplies an equivalent close gate.

## Steps

### Step 1: Inventory Existing Evidence And Missing Artifacts

Goal: Establish what post-contract handoff artifacts already exist and which
acceptance-required docs still need creation or repair.

Actions:
- Inspect the handoff directory and source idea acceptance criteria.
- Map existing scan, bucket, runtime, infrastructure, and follow-up docs.
- Identify missing required docs, especially regression delta and
  `try_gcc_torture` postmortem artifacts.
- Record the first executable documentation packet in `todo.md`.

Completion Check:
- `todo.md` records the artifact inventory, missing docs, and the next packet
  with targeted proof such as `git diff --check`.

### Step 2: Refresh Or Validate Reset-Main Scan Summary

Goal: Ensure the current reset-main RV64 gcc_torture baseline is explicitly
recorded and usable as the comparison anchor.

Actions:
- Review `current_scan_summary.md` for command, date, total/pass/fail counts,
  and scope.
- Refresh or annotate the summary only if the artifact is stale, incomplete, or
  not traceable to reset `main`.
- Keep RV64 gcc_torture outside default CTest expectations.

Completion Check:
- `current_scan_summary.md` is either confirmed sufficient in `todo.md` or
  updated with traceable scan evidence and targeted diff proof.

### Step 3: Write Regression Delta And Branch Postmortem

Goal: Compare reset `main` to `try_gcc_torture` and capture why the exploratory
route should not be replayed wholesale.

Actions:
- Produce or repair `regression_delta.md` with pass/fail deltas between reset
  `main` and `try_gcc_torture`.
- Produce or repair `try_gcc_torture_postmortem.md` covering broad
  improvements, broad regressions, F128 overfit, useful salvage candidates,
  and changes that should be quarantined or rewritten.
- Explicitly record why `conversion.c` and primary-F128 rows are not the main
  route.

Completion Check:
- Both docs exist, include concrete branch-delta evidence, and `todo.md`
  records proof plus any evidence gaps.

### Step 4: Classify Failure Buckets By First Owning Layer

Goal: Make remaining failures actionable by first owner and priority, not by
local testcase novelty.

Actions:
- Review and update `failure_bucket_map.md` against the current scan evidence.
- Separate RV64/MIR lowering, BIR semantic producer, prepared contract, runtime
  mismatch, test infrastructure, stack/global-data, and F128 quarantine rows.
- Rank buckets by broad ordinary-C usefulness and expected case count.
- Ensure primary-F128 rows are screened into quarantine accounting unless they
  demonstrably block non-F128 coverage.

Completion Check:
- The bucket map names first owning layer and priority rationale for each major
  failure family, with F128 quarantined out of ordinary-C repair selection.

### Step 5: Generate Ordered Follow-Up Ideas

Goal: Convert the bucket evidence and postmortem into durable, separately owned
follow-up ideas.

Actions:
- Update `followup_idea_plan.md` with the chosen order and rationale.
- Create missing follow-up ideas under `ideas/open/` for high-priority ordinary
  C buckets.
- Keep the existing F128 policy idea low priority and do not duplicate it
  unless source intent changes.
- Each generated idea must name owning layer, in-scope work, out-of-scope work,
  acceptance criteria, and reviewer reject signals.

Completion Check:
- `followup_idea_plan.md` and generated ideas form an ordered implementation
  queue that separates producer gaps from MIR/RV64 lowering and keeps F128
  quarantined.

### Step 6: Review And Close Readiness

Goal: Decide whether the umbrella source idea is satisfied by the produced
docs, generated ideas, and validation state.

Actions:
- Review every acceptance criterion in the source idea.
- Confirm no implementation patches, expectation downgrades, unsupported
  marker changes, testcase-shaped shortcuts, or F128-priority drift entered the
  umbrella.
- Record close readiness, remaining follow-up paths, and close-gate needs in
  `todo.md`.

Completion Check:
- `todo.md` records close readiness for plan-owner evaluation and the close
  gate has a clear default CTest or supervisor-approved validation path.
