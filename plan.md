# LIR To BIR Global Pointer Aggregate Projection

Status: Active
Source Idea: ideas/open/298_lir_to_bir_global_pointer_aggregate_projection.md
Activated From: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Repair the semantic `lir_to_bir` admission path for global, pointer-derived,
and aggregate projection address formation reached by the AArch64 c-testsuite
backend route.

## Goal

Remove the old residual projection-admission failure for the focused
global/pointer/aggregate family while preserving closed local-memory owner
boundaries and leaving unrelated printer, runtime, and timeout buckets alone.

## Core Rule

Progress must come from semantic projection admission in `lir_to_bir`, not from
filename matching, testcase-number shortcuts, diagnostic-string matching,
expectation rewrites, unsupported downgrades, allowlist changes, timeout
policy changes, runner behavior changes, or CTest registration changes.

## Read First

- `ideas/open/298_lir_to_bir_global_pointer_aggregate_projection.md`
- `ideas/open/295_backend_regex_failure_family_inventory.md`
- `test_before.log`
- Closed local-memory owner 297 before treating any local-memory recurrence as
  reopened scope

## Current Targets

- Accepted broad backend-regex baseline: `test_before.log`, 352 selected, 291
  passed, 61 failed.
- Focused residual projection cases:
  - global scalar-array GEPs: `00176`, `00181`
  - pointer-value or pointer-parameter GEPs: `00182`, `00209`
  - global dynamic aggregate member GEPs: `00195`, `00205`
  - bootstrap/global aggregate semantics: `00204`
  - boundary case: `00216` pointer-parameter/flexible-array aggregate
    projection

## Non-Goals

- Do not touch test expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, or CTest registration.
- Do not repair AArch64 machine-printer residuals, runtime nonzero or runtime
  mismatch buckets, or the standalone `00220` timeout in this plan.
- Do not reopen idea 297 from failing counts alone.
- Do not broadly rewrite unrelated frontend, HIR, LIR, BIR, runtime, or
  AArch64 printer behavior.
- Do not claim completion by making only one named target pass while nearby
  projection cases remain unexamined.

## Working Model

Idea 297 completed direct local-memory admission and left residual global,
pointer, and aggregate projection admissions outside its closure boundary.
This plan owns the remaining compile-stage projection family. The first packet
should inspect the current failing diagnostics and generated artifacts enough
to define the semantic categories, then implementation packets should repair
the shared admission rule without absorbing unrelated failure buckets.

## Execution Rules

- Keep routine packet progress, blockers, and proof in `todo.md`.
- Keep `Current Step ID` and `Current Step Title` aligned with the delegated
  packet.
- Prefer small semantic packets that each build and run the supervisor-selected
  focused backend subset.
- When a focused case stops failing at the old projection-admission point,
  classify any new failure source instead of hiding it.
- If evidence shows `00216` does not share this projection rule, record the
  boundary evidence and request a separate owner instead of stretching this
  plan.
- Treat testcase-overfit and expectation-only count improvements as blocking
  route failures.

## Steps

### Step 1: Inspect Focused Projection Failures

Inspect the focused residual projection cases from `test_before.log` and any
available generated diagnostics or dumps needed to distinguish global scalar
arrays, pointer-derived values, global aggregate members, bootstrap/global
aggregate semantics, and the `00216` boundary case.

Completion check: `todo.md` records the focused cases, their old
`lir_to_bir` projection-admission failure shape, the semantic projection
categories, and whether `00216` appears to share the same owner.

### Step 2: Repair Global And Pointer Projection Admission

Implement the narrow semantic admission needed for global scalar-array GEPs
and pointer-value or pointer-parameter GEPs through the existing lowering
model. Preserve local-memory behavior from idea 297 and avoid named-case or
diagnostic-string matching.

Completion check: the touched code builds, and the supervisor-selected focused
subset shows the global scalar-array and pointer-derived targets no longer
fail at the old projection-admission point.

### Step 3: Repair Aggregate And Bootstrap Projection Admission

Extend the same semantic model to global dynamic aggregate member projections
and bootstrap/global aggregate semantics when the Step 1 evidence shows they
share the owner.

Completion check: the focused aggregate and bootstrap/global targets no longer
fail at the old projection-admission point, or `todo.md` records evidence for
a separate focused owner if one category is distinct.

### Step 4: Decide The `00216` Boundary

Use generated diagnostic evidence to either repair `00216` as part of the same
pointer-parameter/flexible-array aggregate projection rule or classify it as a
separate owner.

Completion check: `00216` is either covered by the repaired projection
admission path or `todo.md` records why it should be split before further
implementation.

### Step 5: Classify Residuals And Prove The Owner

Run the supervisor-selected focused backend subset and classify remaining
focused-scope failures by their new failure source after the old admission
failure is removed.

Completion check: proof records fresh build or compile evidence plus focused
subset results; no remaining focused case is hidden behind the old projection
admission diagnostic; unrelated printer, runtime, and timeout buckets remain
outside this owner.
