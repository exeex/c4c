# Destination Fan-In Authority Research Runbook

Status: Active
Source Idea: ideas/open/607_destination_fan_in_authority_research.md

## Purpose

Produce architecture research documents that decide whether non-parallel move bundles may choose among multiple candidate stack destinations, and what authority rule is required before implementation can proceed.

## Goal

Create `docs/destination_fan_in_authority/` with one index plus three answer files that cite current logs, diagnostics, and prepared/prealloc authority surfaces for the `125` non-parallel multi-source stack-destination rows.

## Core Rule

This plan is documentation-only. Do not change implementation code, tests, expectations, unsupported markers, allowlists, runtime behavior, timeout policy, accounting, or lifecycle state outside the active `plan.md` and `todo.md` updates required during execution.

## Read First

- ideas/open/607_destination_fan_in_authority_research.md
- docs/rv64_gcc_torture_1000_pass_recovery/current_scan_summary.md
- docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md
- ideas/closed/587_prepared_value_freshness_authority_mvp.md
- ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md

## Current Targets

- Owning layer: prepared/prealloc authority.
- Evidence breadth from source idea: `125` non-parallel multi-source stack-destination rows.
- Required output directory: `docs/destination_fan_in_authority/`.
- Required answer files:
  - `docs/destination_fan_in_authority/index.md`
  - `docs/destination_fan_in_authority/01_current_failure_shapes.md`
  - `docs/destination_fan_in_authority/02_destination_authority_rule.md`
  - `docs/destination_fan_in_authority/03_implementation_split.md`
- Proof surface: documentation that cites current scan logs, representative diagnostics, and prepared/prealloc authority surfaces.

## Non-Goals

- Do not implement destination fan-in authority.
- Do not add RV64 target shortcuts for destination choice.
- Do not change tests, expected outputs, unsupported markers, allowlists, runtime behavior, timeout policy, or accounting.
- Do not choose a destination by testcase shape, source order accident, or final assembly convenience.
- Do not merge producer-authority work and RV64 consumption into one follow-up recommendation.

## Working Model

- Treat the `125` row family as an architecture research problem until the destination legality rule is explicit.
- Separate source freshness authority from destination fan-in authority; ideas `587` and `588` are context, not proof that destination choice is legal.
- Use the July 8 RV64 gcc_torture backend evidence as the starting row population unless execution records a newer current scan in `todo.md`.
- If the research cannot define an implementation-ready rule, document the unresolved decision and keep implementation blocked.
- Any follow-up implementation ideas must have a single owner and a proof surface that does not rely on expectation rewrites or named-case shortcuts.

## Execution Rules

- Keep routine progress, selected rows, and evidence notes in `todo.md`; rewrite this runbook only for a route correction.
- Create only the required `docs/destination_fan_in_authority/` files during executor work.
- Every answer file must cite concrete current logs, diagnostics, and code or documentation surfaces used as evidence.
- Preserve exactly three numbered question-answer files plus one `index.md` in the destination authority documentation directory.
- Treat implementation, expectation changes, unsupported-marker changes, allowlist changes, timeout/accounting changes, and RV64 consumer shortcuts as route drift.
- Documentation-only proof should at minimum verify the required file list and check that the answer files contain concrete citations and conclusions.

## Ordered Steps

### Step 1: Document Current Failure Shapes

Goal: identify the current non-parallel multi-source stack-destination family and the evidence that places rows in it.

Primary target:
- `docs/destination_fan_in_authority/01_current_failure_shapes.md`

Actions:
- Inspect the July 8 scan artifacts named by `docs/rv64_gcc_torture_1000_pass_recovery/current_scan_summary.md`.
- Use `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md` as the starting classification, then verify representative rows against current per-case logs.
- Record a representative row table for the `125` row family.
- Capture the diagnostic vocabulary and prepared/prealloc surfaces involved.
- State which rows belong to this family and which nearby rows are adjacent owners instead.

Completion check:
- `01_current_failure_shapes.md` exists, cites the current scan artifacts and representative per-case logs, and concludes the row-family boundary.
- `todo.md` records selected representative rows and any evidence refresh command used.

### Step 2: Decide The Destination Authority Rule

Goal: determine whether the durable legality model is ordering, mutual exclusion, merge authority, or explicit rejection.

Primary target:
- `docs/destination_fan_in_authority/02_destination_authority_rule.md`

Actions:
- Compare destination fan-in evidence against the selected freshness and move-operand source authority from ideas `587` and `588`.
- List the viable alternatives and reject alternatives that depend on testcase shape, source order accident, or target convenience.
- For each viable alternative, identify the producer facts it would require before RV64 consumption.
- Select an implementation-ready destination rule, or explicitly record why the decision remains unresolved.

Completion check:
- `02_destination_authority_rule.md` states alternatives considered, required producer facts, and either a selected authority rule or a documented unresolved decision.
- The conclusion preserves the source-freshness versus destination-authority boundary.

### Step 3: Split Follow-Up Implementation Ownership

Goal: translate the accepted or blocked authority decision into clear next work without starting implementation.

Primary target:
- `docs/destination_fan_in_authority/03_implementation_split.md`

Actions:
- Separate producer-authority work from RV64 consumption work.
- Identify any follow-up implementation ideas that should be opened only after the authority rule is accepted.
- Define proof surfaces for proposed implementation ideas.
- List rows that must remain rejected or blocked until the destination authority rule exists.

Completion check:
- `03_implementation_split.md` separates producer authority from target consumption, names proof surfaces for any proposed follow-up ideas, and records blocked rows when no rule is ready.

### Step 4: Build The Research Index And Acceptance Check

Goal: make the research package complete and easy for the supervisor, reviewer, or plan owner to evaluate.

Primary target:
- `docs/destination_fan_in_authority/index.md`

Actions:
- Summarize the question, evidence base, selected or unresolved destination rule, and implementation split.
- Link the three numbered answer files.
- Verify that the documentation directory contains exactly the required files.
- Verify that no implementation, test, expectation, unsupported-marker, allowlist, runtime, timeout, accounting, or unrelated lifecycle files changed.
- Record the proof commands and closure recommendation in `todo.md`.

Completion check:
- `index.md` exists and links all three numbered answer files.
- The documentation directory contains exactly one `index.md` plus the three required answer files.
- `todo.md` states whether the source idea acceptance criteria are satisfied and whether implementation should remain blocked or move to follow-up idea creation.

## Acceptance Gate

This plan is complete only when `docs/destination_fan_in_authority/` contains exactly `index.md`, `01_current_failure_shapes.md`, `02_destination_authority_rule.md`, and `03_implementation_split.md`; the documents cite concrete current logs, diagnostics, and prepared/prealloc authority surfaces; the conclusion either defines an implementation-ready destination authority rule or records why implementation remains blocked; and no implementation, expectation, unsupported-marker, allowlist, runtime, timeout, accounting, or unrelated lifecycle files are changed.
