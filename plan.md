# Prepared Diagnostics and Oracle Replacement Runbook

Status: Active
Source Idea: ideas/open/194_prepared_printer_debug_oracle_replacement_planning.md

## Purpose

Turn the prepared printer/debug/oracle replacement idea into an executable
planning runbook for route-native diagnostics and equivalent oracle coverage.

## Goal

Define the inventory, replacement diagnostics, oracle coverage, compatibility
needs, and retirement-readiness checklist required before prepared diagnostic
and oracle surfaces can contract.

## Core Rule

Do not remove, privatize, or weaken prepared diagnostics, route-debug output,
or oracle expectations before a route-native replacement and equivalent
positive, negative, ambiguous, and fallback coverage are explicitly planned.

## Read First

- `ideas/open/194_prepared_printer_debug_oracle_replacement_planning.md`
- Existing prepared printer, debug, route-debug, dump, and oracle tests that
  consume prepared route facts or `PreparedFunctionLookups`
- Current selected-consumer route-view migration notes and tests relevant to
  retained diagnostic or oracle surfaces

## Current Targets

- Prepared printer consumers
- Prepared debug and route-debug consumers
- Dump surfaces that expose prepared route facts
- Oracle tests that depend on prepared route facts or `PreparedFunctionLookups`
- Compatibility adapters needed while prepared and route-native diagnostics
  coexist

## Non-Goals

- Do not remove existing prepared diagnostics.
- Do not replace production lowering behavior in this planning route.
- Do not delete oracle tests or weaken expected output.
- Do not create target-specific route adapters only for diagnostic convenience.
- Do not open draft 155 or demote `PreparedBirModule` fields.
- Do not claim production lowering success or full-suite greenness as
  diagnostic/oracle readiness.

## Working Model

Prepared diagnostics and oracles remain public blockers until each retained
surface has a route-native replacement plan or an explicit retained-oracle
rationale. The output of this runbook is a durable planning artifact and a
split-ready checklist for future implementation ideas, not implementation code.

## Execution Rules

- Keep the route focused on diagnostics and oracle planning.
- Preserve source intent in the idea file; routine progress belongs in
  `todo.md`.
- Name concrete consumers and files where possible.
- Separate route-native semantic diagnostics from target-local policy,
  formatting, or compatibility fallback.
- Treat expectation rewrites, label-only changes, and named-test shortcuts as
  route drift.
- If implementation work appears necessary, record the implementation need as
  a future idea instead of doing it under this runbook.

## Ordered Steps

### Step 1: Inventory Prepared Diagnostic And Oracle Consumers

Goal: identify every retained prepared printer, debug, route-debug, dump, and
oracle consumer that blocks prepared API contraction.

Primary target: prepared diagnostic/oracle call sites and tests that read
prepared route facts or `PreparedFunctionLookups`.

Actions:

- Inspect printer, debug, route-debug, dump, oracle, wrapper, and expected
  output consumers.
- Group consumers by surface and route family.
- Mark whether each consumer is production-adjacent, debug-only,
  route-debug-only, dump-only, oracle-only, or compatibility-only.
- Name any consumer whose ownership is unclear instead of guessing.

Completion check:

- The planning artifact contains a consumer inventory with enough file/function
  detail for a later executor to split implementation by surface.

### Step 2: Define Route-Native Replacement Diagnostics

Goal: state what route-native diagnostic output must exist before each
prepared diagnostic surface can contract.

Primary target: each retained prepared printer/debug/route-debug/dump surface
from Step 1.

Actions:

- For each surface, identify the route facts it currently exposes.
- Define the route-native equivalent output or explain why the prepared oracle
  must remain.
- Separate semantic route facts from formatting, target policy, and prepared
  compatibility facts.
- Note any temporary compatibility adapter needed while both surfaces coexist.

Completion check:

- Every retained diagnostic surface has either a route-native replacement plan
  or an explicit retained-oracle rationale.

### Step 3: Define Equivalent Oracle Coverage

Goal: specify the positive, negative, ambiguous, and fallback oracle coverage
required for each affected route family.

Primary target: oracle and expected-output coverage tied to prepared route
facts, route-debug output, wrappers, and fallback behavior.

Actions:

- Identify existing oracle coverage by route family and diagnostic surface.
- Record required positive, negative, ambiguous, mismatch, and fallback cases.
- Call out coverage gaps that must block contraction.
- Preserve existing oracle strength; do not weaken expectations to match
  current implementation gaps.

Completion check:

- The planning artifact names equivalence coverage required before each
  prepared diagnostic or oracle surface can contract.

### Step 4: Build The Retirement-Readiness Checklist

Goal: produce a checklist that future implementation ideas can use without
collapsing into a broad prepared API retirement rewrite.

Primary target: durable diagnostic/oracle retirement-readiness artifact.

Actions:

- Summarize blockers by surface, route family, and consumer group.
- List compatibility adapter needs and retained-oracle rationales.
- Split future implementation candidates by diagnostic/oracle surface.
- Include reject signals for expectation weakening, output-only relabeling, and
  testcase-shaped diagnostic shortcuts.

Completion check:

- Future implementation ideas can be scoped to one diagnostic or oracle surface
  at a time, and the checklist makes prepared contraction prerequisites clear.

### Step 5: Validate Planning Consistency

Goal: verify the planning artifact satisfies the source idea without changing
implementation behavior.

Primary target: the generated diagnostic/oracle planning artifact and lifecycle
  state.

Actions:

- Re-read the source idea acceptance criteria and reviewer reject signals.
- Confirm no implementation files, expected-output files, or closed idea files
  were changed as part of planning.
- Confirm the artifact does not treat production lowering success as
  diagnostic/oracle readiness.
- If the planning artifact creates follow-up ideas, keep each one scoped to a
  single diagnostic/oracle surface.

Completion check:

- The plan can be accepted as diagnostics/oracle readiness planning without
  claiming prepared API contraction or route capability progress.
