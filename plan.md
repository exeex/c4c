# Calls Preservation Republication Retirement Runbook

Status: Active
Source Idea: ideas/open/09_calls_preservation_republication_retirement.md

## Purpose

Retire target-local AArch64 preservation and republication reconstruction by
moving durable call-preservation facts into prepared call planning.

## Goal

AArch64 call emission consumes prepared preservation and republication facts
instead of reconstructing which values survive call clobbers from raw BIR shape,
value-home scans, or late target guesses.

## Core Rule

Do not trade one AArch64 reconstruction path for another. Preservation source,
destination, and republication authority must live in prepared records with a
clear conflict rule; AArch64 may only emit target instructions from those
complete facts.

## Read First

- `ideas/open/09_calls_preservation_republication_retirement.md`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- AArch64 call emission helpers that preserve values across calls
- prepared call-plan records and lookup helpers under `src/backend/mir`
- focused tests for call clobbers, stack-preserved values, later scalar use,
  and repeated call arguments

## Current Targets

- Preservation and post-call republication logic used by AArch64 calls.
- Prepared records that can carry source, destination, and republication
  reasons.
- AArch64 helpers that currently infer preserved values after planning.
- Focused backend and c_testsuite call-preservation coverage selected by the
  supervisor.

## Non-Goals

- Do not redesign the full register allocator.
- Do not clean up dispatch edge-copy behavior outside preservation consumers.
- Do not perform assembly-printer-only cleanup; that belongs to idea `10`.
- Do not consolidate the calls file family; that belongs to idea `11`.
- Do not weaken tests, diagnostics, or c_testsuite expectations.

## Working Model

- Prepared call planning should decide which live values must survive call
  clobbers and where those values are available afterward.
- AArch64 preservation code should translate prepared preservation records into
  concrete moves, stores, reloads, and post-call publications.
- Later non-call scalar uses and later call-argument reuse are both consumers of
  the same preservation truth.
- Any remaining raw BIR scan or broad value-home fallback in AArch64 is suspect
  unless it is strictly target operand construction from a prepared fact.

## Execution Rules

- Audit each preservation path before moving it; identify whether it chooses a
  semantic source, selects a target location, republishes a value, or emits an
  instruction.
- Keep prepared authority and AArch64 emission authority separated in names,
  signatures, and tests.
- Add focused proof for each newly prepared fact before deleting the target
  fallback it replaces.
- Preserve cross-block behavior; do not validate only adjacent single-block
  call cases.
- Treat broad post-call overwrite/reload fallbacks, named-case matching, and
  expectation downgrades as route failures.

## Ordered Steps

### Step 1: Audit Preservation And Republication Authority

Goal: identify every AArch64 path that reconstructs preservation or
republication semantics after call planning.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- prepared call-plan preservation records and lookup helpers
- focused clobber, stack-preserved, later-use, and repeated-call-argument tests

Actions:

- List every helper and call site that decides which value must be preserved,
  where it is stored, where it is restored from, or how it is republished.
- Classify each decision as prepared semantic authority, AArch64 target
  location choice, or instruction emission.
- Identify any target-local broad value-home scan or raw BIR inference that
  should become a prepared fact.
- Record coverage gaps for clobbered registers, stack-preserved values,
  cross-block later uses, and later call-argument reuse.

Completion check:

- The executor can name which preservation and republication decisions must
  move into prepared records, which target helpers remain emission-only, and
  which tests prove each behavior family.

### Step 2: Prepare Preservation Source And Destination Facts

Goal: make prepared call records carry the facts needed to preserve live values
across calls.

Primary targets:

- prepared call-plan record definitions
- prepared call-plan construction and validation
- consumers that need preserved values after call clobbers
- focused preservation-source tests

Actions:

- Add or extend prepared records for the preserved value, the chosen source,
  the preservation destination, and the reason the value must survive the call.
- Move semantic diagnostics for missing or conflicting preservation facts into
  prepared planning.
- Keep AArch64-specific register names, stack addressing, and move emission out
  of the prepared semantic record unless they are already target facts by
  design.
- Add focused tests for clobbered register preservation and stack-preserved
  values before relying on the new facts from AArch64.

Completion check:

- Prepared planning can explain which value is preserved, from where, to where,
  and why, without asking AArch64 to rediscover that semantic relationship.

### Step 3: Prepare Post-Call Republication Facts

Goal: make later consumers reload or reuse preserved values from prepared
republication records instead of AArch64 post-call guesses.

Primary targets:

- prepared post-call republication records or lookup helpers
- AArch64 call-boundary consumer code
- tests for later non-call scalar use and later call-argument reuse

Actions:

- Define the prepared fact that tells later consumers which preserved value is
  republished after the call and which home is authoritative.
- Replace target-local post-call overwrite or reload reconstruction with
  prepared republication lookups.
- Preserve behavior for later scalar uses and for values that become arguments
  to a later call.
- Add or tighten focused tests for same-block and cross-block consumers.

Completion check:

- Later consumers get preserved values through prepared republication facts,
  and AArch64 no longer owns the semantic choice of which post-call value home
  should win.

### Step 4: Shrink AArch64 Preservation Helpers To Emission

Goal: remove duplicate preservation authority from AArch64 and leave only
target instruction emission.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- AArch64 call helper declarations and local adapters
- build metadata if helper files or declarations are retired

Actions:

- Redirect AArch64 preservation helpers to consume prepared preservation and
  republication records.
- Delete or localize helpers whose only purpose was recovering semantic
  preservation facts.
- Keep target-specific emission details such as register moves, stack slots,
  and instruction sequencing local to AArch64.
- Update declarations, includes, and build metadata in the same packet as any
  helper removal.

Completion check:

- AArch64 no longer reconstructs preservation or republication semantics from
  raw BIR shape or broad value-home scans, and the remaining helper surface is
  emission-oriented.

### Step 5: Validate Preservation Retirement

Goal: prove that prepared preservation authority covers the old behavior
without weakening tests or adding testcase-shaped shortcuts.

Primary targets:

- focused prepared-call preservation tests
- AArch64 call clobber and later-use backend tests
- representative c_testsuite call-preservation cases selected by the supervisor

Actions:

- Run build or compile proof after each code-changing packet.
- Run focused tests for clobbered registers, stack-preserved values,
  cross-block preservation, later scalar use, and later call-argument reuse.
- Run representative c_testsuite call-preservation cases after removing
  target-local reconstruction paths.
- Escalate to supervisor-selected broader validation before treating the
  retirement as complete.

Completion check:

- Focused and representative call-preservation tests pass, prepared records
  explain the surviving values and republication homes, and no unsupported
  markings, expectation weakening, broad reload fallback, or named-case
  shortcut was introduced.
