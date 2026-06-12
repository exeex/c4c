# Phase E2 fused compare operand producer helper private pass-context

Status: Active
Source Idea: ideas/open/225_phase_e2_fused_compare_operand_producer_helper_private_pass_context.md

## Purpose

Contract the public prepared helper family around
`find_prepared_fused_compare_operand_producer_facts(...)` so the selected
AArch64 comparison-provenance identity read uses a private pass-context
boundary, while preserving prepared and AArch64 ownership for fallback,
branch policy, printer/debug output, wrappers, helper-oracle behavior, and
expected strings.

## Goal

Move only the agreement-proven Route 7 fused-compare operand producer identity
read for the AArch64 conditional-branch consumer behind private pass-context
access, with focused proof that all non-agreement and target-policy behavior is
unchanged.

## Core Rule

Route 7 owns only the selected comparison-provenance identity read under
route/prepared agreement. Prepared and AArch64 target lowering retain authority
for absent-route, invalid-reference, duplicate/conflict, mismatch, unfused,
non-agreement, branch target, suffix, legality, hazard, emitted-register,
assembler-order, printer/debug, wrapper, helper-oracle, and expected-string
behavior.

## Read First

- `ideas/open/225_phase_e2_fused_compare_operand_producer_helper_private_pass_context.md`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- Existing prepared lookup/helper definitions around
  `find_prepared_fused_compare_operand_producer_facts(...)`

## Current Targets

- Public callers and immediate helper family for
  `find_prepared_fused_compare_operand_producer_facts(...)`
- AArch64 comparison lowering in
  `src/backend/mir/aarch64/codegen/comparison.cpp`
- Focused prepared-helper tests in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Non-Goals

- Do not start broad Route 1 through Route 7 migration.
- Do not add generic route-index facades or replace aggregate route views.
- Do not retire `PreparedFunctionLookups` or `PreparedBirModule`.
- Do not move branch target selection, suffix mapping, fused legality, hazard
  handling, emitted-register state, final instruction order, final assembler
  rows, wrappers, printer/debug rows, helper-oracle strings, or expected
  strings into route authority.
- Do not use facade renames, wrapper moves, construction reshuffles, baseline
  refreshes, unsupported downgrades, timeout masking, or expectation rewrites
  as proof.

## Working Model

- The public helper remains responsible for prepared fallback behavior until
  every public production, wrapper, printer/debug, oracle, and expected-string
  consumer is either migrated or explicitly retained.
- The new private boundary should expose only the selected
  Route 7/prepared-agreement identity read needed by AArch64 comparison
  lowering.
- Non-agreement must fail closed to the existing prepared behavior.
- Target branch policy and final output remain byte-stable.

## Execution Rules

- Keep each step behavior-preserving unless the source idea explicitly allows
  a semantic change.
- Prefer narrow source edits and focused tests over broad helper reshaping.
- Preserve helper-oracle names, statuses, and failure-mode strings.
- Preserve final assembler output, printer/debug output, wrappers, and expected
  strings.
- For code-changing steps, run a fresh build plus the supervisor-delegated
  focused backend subset and write the result to `test_after.log`.
- Escalate to a broader backend proof before closeout.

## Steps

### Step 1: Inventory Public Consumers

Goal: account for every public caller and retained consumer for the fused
compare operand producer helper family.

Primary targets:

- `find_prepared_fused_compare_operand_producer_facts(...)`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:

- Inspect direct callers of
  `find_prepared_fused_compare_operand_producer_facts(...)` and its immediate
  helper-family support code.
- Classify each caller as production comparison-lowering, target branch policy,
  wrapper, printer/debug, helper-oracle, expected-string, aggregate lookup, or
  test-only coverage.
- Record in `todo.md` which consumers are candidates for private pass-context
  migration and which must stay on public prepared surfaces.
- Do not edit implementation files in this step.

Completion check:

- `todo.md` lists all discovered public consumers and states why each is
  migrated or retained.
- No source, test, expected-string, or lifecycle source-idea files are changed.

### Step 2: Extract The Private Agreement Reader

Goal: move only the selected Route 7/prepared-agreement fused compare operand
producer identity read behind a private pass-context boundary.

Primary targets:

- Prepared helper-family implementation for
  `find_prepared_fused_compare_operand_producer_facts(...)`
- Any nearby private `detail` namespace or pass-context style used by the
  prepared lookup helpers

Actions:

- Add a private pass-context type or reuse an existing private boundary that
  can read the selected Route 7 comparison-provenance operand producer facts.
- Gate the private read on agreement with retained prepared facts.
- Preserve public helper fallback for absent route data, invalid references,
  duplicates/conflicts, mismatches, unfused cases, and non-agreement.
- Do not change branch target selection, suffix mapping, fused legality,
  hazards, emitted-register state, final assembler order, printer/debug rows,
  wrappers, helper-oracle strings, or expected strings.

Completion check:

- The selected identity read is available through a private pass-context
  boundary.
- Public fallback behavior is retained.
- Fresh build plus the supervisor-delegated focused backend subset passes and
  is recorded in `test_after.log`.

### Step 3: Migrate The AArch64 Comparison-Lowering Consumer

Goal: make the selected AArch64 conditional-branch comparison consumer use the
private pass-context boundary for the agreement-proven identity read.

Primary target:

- `src/backend/mir/aarch64/codegen/comparison.cpp`

Actions:

- Update the selected consumer inside the conditional-branch comparison
  lowering path to call the private boundary after retained prepared fallback
  lookup.
- Keep target branch policy, suffix mapping, legality checks, hazards,
  emitted-register state, final instruction order, and final assembler rows in
  AArch64 lowering.
- Retain two-argument prepared fallback and any public helper-oracle or wrapper
  surface needed by tests and non-production consumers.
- Do not change x86 behavior, printer/debug output, aggregate lookup surfaces,
  or expected strings.

Completion check:

- The production comparison-lowering caller no longer performs the duplicate
  public semantic identity read for this helper family.
- All retained public surfaces remain explicitly justified in `todo.md`.
- Fresh build plus the supervisor-delegated focused backend subset passes and
  is recorded in `test_after.log`.

### Step 4: Add Focused Helper-Family Proof

Goal: prove the selected contraction and all required fallback behavior without
reshaping expectations.

Primary target:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:

- Add or update direct private-boundary coverage for positive agreement.
- Preserve or add focused coverage for absent-route, invalid-reference,
  duplicate/conflict, mismatch, unfused, and non-agreement fallback.
- Include at least one adjacent comparison-provenance or
  materialized-condition case so the proof is not shaped around one testcase.
- Preserve helper-oracle names, statuses, failure-mode strings, wrappers,
  supported-path status, printer/debug output, and expected strings.

Completion check:

- The focused helper tests cover agreement and required fallback modes.
- No unsupported downgrades, baseline refreshes, timeout masking, or
  expected-string rewrites are present.
- Fresh build plus the supervisor-delegated focused backend subset passes and
  is recorded in `test_after.log`.

### Step 5: Broader Backend Proof And Closeout Readiness

Goal: prove the helper-family contraction did not disturb nearby backend
behavior and prepare lifecycle closeout.

Actions:

- Run a broader backend proof chosen by the supervisor, normally all
  `backend_` tests.
- Check the diff for accidental scope expansion into aggregate route migration,
  prepared aggregate retirement, branch target policy, printer/debug output,
  wrappers, helper-oracle strings, or expected strings.
- Update `todo.md` with final consumer accounting, proof command, result, and
  any retained public surfaces.
- Do not edit implementation files in this step unless the broader proof finds
  a real defect that belongs to this source idea.

Completion check:

- Broader backend proof passes and is recorded in `test_after.log`.
- `todo.md` is ready for plan-owner closure review.
- The source idea acceptance proof is satisfied without testcase-shaped
  shortcuts or expectation rewrites.
