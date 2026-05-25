# Prepared Call Argument Source Selection Completeness Runbook

Status: Active
Source Idea: ideas/open/05_prepared_call_argument_source_selection_completeness.md

## Purpose

Complete `PreparedCallArgumentSourceSelection` so call argument emission can
consume structured prepared facts without target-local fallback rederivation.

## Goal

Make every prepared call argument source selection kind declare and carry the
facts required by emission, with prior-preservation selections covering both
stack-slot and callee-saved-register sources.

## Core Rule

Complete the prepared selection contract. Do not restore broad AArch64
source-reconstruction fallback or make a narrow c_testsuite-shaped shortcut.

## Read First

- `ideas/open/05_prepared_call_argument_source_selection_completeness.md`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- Shared prepared call-plan records and
  `PreparedCallArgumentSourceSelectionKind` definitions.
- Prepared call selection printers, diagnostics, and tests.
- Recent prior-preservation/source-selection repairs from the `04` family for
  expected behavior and anti-overfit boundaries.

## Current Targets

- `PreparedCallArgumentSourceSelectionKind` and all associated payload records.
- Prior-preservation source selections for stack-slot and
  callee-saved-register sources.
- AArch64 call argument emission consumers that still need unrelated lookups
  because prepared selections are incomplete.
- Printer and diagnostic surfaces that should expose missing or incomplete
  prepared facts.

## Non-Goals

- Do not consolidate the AArch64 calls file family; that belongs to later
  ideas.
- Do not move preservation source authority wholesale; that belongs to
  `ideas/open/06_prepared_call_preservation_source_authority.md`.
- Do not rewrite dispatch or value materialization ownership.
- Do not change call ABI classification except where a missing prepared source
  fact proves the current contract is incomplete.
- Do not weaken tests, mark supported paths unsupported, or hide incomplete
  selections behind target-local reconstruction.

## Working Model

The prior-preservation baseline-drift work showed that a selection can name a
semantic source choice without carrying every fact AArch64 emission needs.
Stack-slot prior preservation already has explicit selection fields, while
callee-saved prior preservation can still require emission to search preserved
value records. This plan closes that asymmetry at the prepared-record layer.

## Execution Rules

- Audit the prepared selection contract before changing consumers.
- Prefer ABI-neutral prepared facts and target-policy boundaries over embedding
  AArch64 register details in shared records.
- Make incomplete selections fail closed through diagnostics or no selection,
  not through silent fallback rederivation.
- Keep target-specific operand construction in AArch64, but remove target-local
  semantic source reconstruction when prepared facts can carry it.
- Record packet progress and exact proof commands in `todo.md`; do not edit
  the source idea for routine execution notes.
- Escalate to plan-owner if the audit proves the missing authority belongs to
  idea `06` rather than this selection-completeness contract.

## Steps

### Step 1: Audit Selection Kinds And Consumers

- Enumerate all `PreparedCallArgumentSourceSelectionKind` variants and their
  payload fields.
- For each variant, list the facts emission needs and whether the prepared
  selection currently carries them directly.
- Inspect AArch64 consumers for fallbacks or lookups that exist only because a
  prepared selection omits required source facts.
- Identify printer and diagnostic gaps that make incomplete selections hard to
  see.

Completion check: `todo.md` records a variant-by-variant completeness table,
the first incomplete selection family, and the narrow consumer path that proves
the missing fact is required.

### Step 2: Complete Prior-Preservation Selection Facts

- Add the missing structured facts or subrecords needed for prior-preservation
  selections to represent stack-slot and callee-saved-register sources.
- Keep shared records ABI-neutral unless a clear target-policy boundary owns
  the target detail.
- Update construction sites so complete selections are produced only when all
  required facts are available.
- Ensure incomplete or ambiguous selections fail closed instead of producing a
  partially usable record that AArch64 must repair later.

Completion check: prepared records can represent both stack-slot and
callee-saved prior-preservation sources without requiring AArch64 to search
unrelated prior homes.

### Step 3: Update Emission, Printing, And Diagnostics

- Change AArch64 emission consumers to read the completed prepared facts.
- Remove or narrow target-local fallback rederivation that is obsolete after
  Step 2.
- Update prepared printers and diagnostics so complete and incomplete
  selections show the relevant source facts.
- Preserve target-specific register views and operand construction inside
  AArch64.

Completion check: emission uses the completed selection payloads, diagnostics
make missing fields visible, and no new broad prior-home fallback is introduced.

### Step 4: Add Focused Coverage

- Add or update tests for complete stack-slot prior-preservation selection.
- Add or update tests for complete callee-saved-register prior-preservation
  selection.
- Add or update tests for intentionally incomplete selections failing closed.
- Include printer or diagnostic checks when they are the best way to prove
  missing fields are visible.

Completion check: focused tests fail before the contract repair and pass after
it, without relying on named c_testsuite cases as implementation conditions.

### Step 5: Validate Selection Completeness

- Run the supervisor-delegated build and focused backend proof.
- Run representative prior-preservation runtime cases from the `04` family if
  the changed surface can affect them.
- Run any focused printer or diagnostic tests updated in Step 4.
- Record exact commands, results, and proof log paths in `todo.md`.

Completion check: build and focused tests pass; representative
prior-preservation cases remain green; the final diff contains no unsupported
expectation downgrades, named-case shortcuts, or restored broad fallback.
