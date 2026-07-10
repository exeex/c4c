# LLVM Torture 20040709 Owner Discovery Runbook

Status: Active
Source Idea: ideas/open/668_llvm_torture_20040709_research.md

## Purpose

Produce focused owner-discovery documentation for the two LLVM torture
`20040709` rows without making implementation, expectation, runtime, or
baseline-policy changes.

Goal: decide whether `llvm_gcc_c_torture_src_20040709_2_c` and
`llvm_gcc_c_torture_src_20040709_3_c` map to an existing follow-up idea or
need a separate frontend, runtime, backend, or harness follow-up.

## Core Rule

This is a research plan. Do not edit compiler code, test expectations,
unsupported markers, allowlists, timeout policy, runtime behavior, baseline
acceptance, or lifecycle history while executing it.

## Read First

- `ideas/open/668_llvm_torture_20040709_research.md`
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
- Current baseline logs and any focused reproduction output needed to cite the
  first observable failure boundary.

## Current Targets

- `llvm_gcc_c_torture_src_20040709_2_c`
- `llvm_gcc_c_torture_src_20040709_3_c`
- Required output directory:
  `docs/llvm_torture_20040709_owner_discovery/`

## Non-Goals

- Do not implement a fix.
- Do not change backend, frontend, runtime, test, harness, expectation,
  unsupported-marker, allowlist, timeout, or baseline-accounting behavior.
- Do not assign either row to RV64, prepared CLI, AArch64, object emission, or
  another owner without focused row-specific evidence.
- Do not collapse the required numbered answer files into only an index.

## Working Model

The two LLVM torture rows were intentionally deferred because the current
evidence did not prove a first implementation owner. Execution should gather
enough evidence to classify each row's first observable boundary, then compare
that evidence against existing generated follow-up ideas before recommending a
new direct implementation idea.

## Execution Rules

- Keep documentation output exactly scoped to
  `docs/llvm_torture_20040709_owner_discovery/`.
- Each numbered answer file must answer only its assigned question.
- Cite concrete logs, commands, code surfaces, or existing follow-up ideas for
  every owner claim.
- Treat named-case shortcuts, harness filtering, and expectation edits as route
  drift, not progress.
- If focused reproduction is run, record commands and results in the relevant
  documentation file and in `todo.md` proof notes.

## Step 1: Establish Current Failure Boundary

Goal: determine the first observable failure boundary for both LLVM torture
rows.

Primary target:
`docs/llvm_torture_20040709_owner_discovery/01_current_failure_boundary.md`

Actions:

- Inspect the current baseline log and triage docs for rows 1941 and 1942.
- Run only focused reproduction commands needed to identify the first
  boundary, if existing logs are insufficient.
- Classify each row as frontend, prepared backend, target backend, runtime, or
  harness boundary.
- State whether either row has enough evidence for implementation.

Completion check:

- `01_current_failure_boundary.md` cites the current baseline and any focused
  reproduction command used, classifies both rows, and separates evidence from
  inference.

## Step 2: Map Rows To Existing Follow-Ups

Goal: decide whether each row maps to an existing generated follow-up idea or
requires a new direct implementation idea.

Primary target:
`docs/llvm_torture_20040709_owner_discovery/02_owner_mapping.md`

Actions:

- Compare each row's Step 1 boundary against generated follow-up ideas and the
  follow-up order document.
- Name a shared owner only when concrete evidence proves the same first owner.
- If no existing owner fits, propose a separate direct implementation idea with
  the minimum durable scope needed.

Completion check:

- `02_owner_mapping.md` directly answers the mapping question for both rows,
  links owner claims to evidence, and avoids broad unproven follow-up
  recommendations.

## Step 3: Build Research Index And Validate Scope

Goal: finish the required documentation package and prove no forbidden files
were changed.

Primary target:
`docs/llvm_torture_20040709_owner_discovery/index.md`

Actions:

- Create `index.md` linking to both numbered answer files.
- Summarize the overall result without replacing either answer file.
- Verify the output directory contains exactly:
  - `index.md`
  - `01_current_failure_boundary.md`
  - `02_owner_mapping.md`
- Check `git diff --name-only` and ensure execution changes stay within the
  research docs plus routine `todo.md` progress.

Completion check:

- The required three documentation files exist, all source-idea acceptance
  criteria are satisfied, and no implementation, expectation, unsupported,
  allowlist, runtime, baseline-policy, or lifecycle-history file was changed by
  execution.
