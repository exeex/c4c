# Route 7 Comparison Provenance Diagnostic Oracle Runbook

Status: Active
Source Idea: ideas/open/206_route7_comparison_provenance_diagnostic_oracle.md

## Purpose

Activate the Route 7 follow-up by replacing or adding one visible
comparison-provenance diagnostic/oracle row while keeping target branch policy
and prepared fallback authority outside BIR schema.

Goal: one selected comparison provenance reader or visible diagnostic/oracle row
obtains Route 7 comparison provenance from BIR, with prepared fallback preserved
for policy-sensitive branch, fused-compare, debug, printer, and handoff
surfaces.

Core Rule: Route 7 may provide comparison provenance only; it must not become
the authority for branch emission, fused legality, condition-code choice, branch
spelling, hazards, emitted-register state, final instruction order, final
instruction records/errors, or AArch64 compare/branch selection.

## Read First

- `ideas/open/206_route7_comparison_provenance_diagnostic_oracle.md`
- Existing Route 7 comparison records, indexes, publication sites, and readers
- Existing comparison, fused-compare, materialized-condition, branch-condition,
  route-debug, handoff, printer/debug, and target branch-policy tests
- Nearby expected-string or diagnostic oracle tests for the selected surface

## Current Scope

- Select exactly one comparison provenance reader or visible diagnostic/oracle
  row for the first Route 7 adapter.
- Use Route 7 records or indexes only for comparison-producing instruction,
  condition value/name, lhs/rhs producer nodes or integer constants, producer
  instruction index, fused-operand provenance, materialized-condition
  provenance, and branch-condition provenance.
- Preserve prepared fallback for fused compare, materialized condition,
  unfused fallback, route debug/handoff, printer/debug, and target branch-policy
  oracle surfaces.
- Prove fused, materialized, unfused fallback, absent-route,
  invalid-reference, duplicate, mismatch, and unchanged expected-string
  behavior.

## Non-Goals

- Do not perform branch-policy cleanup or branch emission migration.
- Do not move fused-compare legality, condition-code choice, branch spelling,
  hazards, emitted-register state, final instruction order, final instruction
  records/errors, or AArch64 compare/branch selection into BIR schema.
- Do not weaken prepared printer/debug, route-debug, string-authority, or target
  branch-policy coverage.
- Do not rewrite expected strings, downgrade supported tests, or rename helpers
  and claim that as capability progress.

## Working Model

- Treat Route 7 as a provenance adapter for one selected comparison reader or
  visible oracle row.
- Treat prepared comparison/branch machinery as the authority for policy,
  legality, final branch selection, emitted-register state, instruction order,
  and final output.
- Fail closed when Route 7 facts are absent, invalid, duplicated, ambiguous,
  mismatched, policy-bound, or not applicable to the selected surface.
- Keep the first implementation narrow: one visible provenance surface, one
  proof ladder, then supervisor review before broadening.

## Execution Rules

- Keep implementation steps small enough for build plus targeted backend tests.
- Prefer existing Route 7 lookup helpers, prepared fallback helpers, and nearby
  oracle test style.
- Add no testcase-shaped shortcuts, named-fixture branches, or string rewrites
  that mask unchanged capability.
- If diagnostic or expected-output strings are touched, prove the changed row
  directly and preserve adjacent string-authority coverage.
- After each code-changing packet, run the supervisor-delegated build/test
  command exactly and record proof in `todo.md`.
- Escalate to reviewer scrutiny before accepting any slice that appears to
  migrate branch policy or compare/branch selection instead of provenance.

## Ordered Steps

### Step 1: Select The Route 7 Provenance Surface

Goal: identify exactly one comparison provenance reader or visible
diagnostic/oracle row for the first Route 7 adapter.

Primary target: one existing comparison provenance, diagnostic, debug, handoff,
or oracle reader.

Actions:

- Inspect Route 7 publication records, indexes, and existing prepared comparison
  provenance readers.
- Choose the smallest visible surface that needs comparison provenance and
  already has prepared fallback for branch-policy or output-sensitive behavior.
- Map which facts may come from Route 7 and which facts must remain prepared.
- Identify targeted tests and any missing negative coverage for fused,
  materialized, unfused fallback, absent-route, invalid-reference, duplicate,
  mismatch, and unchanged expected-string behavior.

Completion check:

- `todo.md` names one selected provenance reader or visible oracle row, the
  Route 7 facts it may read, the prepared facts it must keep, and the narrow
  proof command for Step 2.

### Step 2: Add The Fail-Closed Route 7 Reader Adapter

Goal: introduce the Route 7 provenance adapter without changing prepared
branch-policy or compare/branch authority.

Primary target: the selected reader and its nearest Route 7 lookup helpers.

Actions:

- Add or reuse a helper that validates the selected Route 7 comparison
  provenance.
- Reject absent facts, invalid references, duplicate or ambiguous facts,
  mismatched condition values/names, mismatched producer indexes, mismatched
  lhs/rhs producer or constant facts, and policy-bound facts for the selected
  surface.
- Return to prepared fallback for fused-compare legality, condition-code
  selection, branch spelling, hazards, emitted-register state, final
  instruction order, final records/errors, route debug/handoff, printer/debug,
  and target branch-policy oracles.
- Keep existing prepared helper groups and broad lookup surfaces intact.

Completion check:

- The project builds.
- Targeted proof covers valid Route 7 provenance, invalid-reference rejection,
  duplicate rejection, mismatch fallback, absent-route fallback, and preserved
  prepared fallback for the selected surface.

### Step 3: Wire The Selected Diagnostic Or Oracle Row

Goal: make the selected visible surface consume Route 7 comparison provenance
when valid and prepared fallback when invalid, absent, or policy-bound.

Primary target: the selected reader or visible diagnostic/oracle row from Step
1.

Actions:

- Thread the validated Route 7 provenance into the selected reader or oracle
  row.
- Preserve prepared authority for branch policy, fused legality, final
  compare/branch selection, hazards, emitted-register state, instruction order,
  final records/errors, and output strings outside the selected provenance row.
- Add or update focused tests for fused provenance, materialized-condition
  provenance, unfused fallback, absent-route fallback, invalid references,
  duplicate facts, mismatches, and unchanged adjacent expected strings.

Completion check:

- The selected visible surface obtains only Route 7 comparison provenance from
  BIR.
- Prepared comparison/branch machinery remains visibly authoritative for policy
  and output-sensitive behavior.
- Targeted tests pass with no unsupported downgrades or expected-string
  weakening.

### Step 4: Prove Adapter Completeness And Route Quality

Goal: establish that the adapter is real comparison-provenance progress and not
a narrow fixture shortcut.

Primary target: tests and review evidence for the selected Route 7 surface.

Actions:

- Run the supervisor-selected build and targeted backend test subset.
- Add or run nearby same-feature cases to show the adapter is not
  testcase-shaped.
- Include expected-string, printer/debug, route-debug/handoff, or branch-policy
  oracle sanity when the selected surface is near those paths.
- Document remaining non-goals and any proposed next Route 7 boundary in
  `todo.md` without expanding this plan.

Completion check:

- Proof covers fused, materialized, unfused fallback, absent-route,
  invalid-reference, duplicate, mismatch, and unchanged expected-string
  behavior.
- No branch-policy cleanup, branch emission migration, fused-legality migration,
  compare/branch selection migration, expected-string weakening, unsupported
  downgrade, or helper-rename-only progress is present.
- The supervisor can decide whether to accept the slice, request reviewer
  scrutiny, or activate a separate follow-up idea.
