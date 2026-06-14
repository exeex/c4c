# Phase F1 x86 Route 6 Call-Wrapper Diagnostic/Oracle Replacement

Status: Active
Source Idea: ideas/open/244_phase_f1_x86_route6_call_wrapper_diagnostic_oracle_replacement.md

## Purpose

Replace x86 direct-call wrapper semantic source authority for the selected
scalar `i32` call argument path with Route 6 facts while preserving prepared
compatibility, diagnostics, helper-oracle behavior, fallback behavior, wrapper
assembly, and supported-path contracts.

## Goal

Make Route 6 the named semantic source authority for the selected scalar `i32`
x86 direct-call argument source identity only where it agrees with the existing
prepared call argument source selection.

## Core Rule

Do not broaden this into x86 call-wrapper migration. This plan is a narrow
agreement-gated Route 6 diagnostic/oracle replacement for one scalar `i32`
source identity path, with prepared fallback and compatibility surfaces kept
observable.

## Read First

- `ideas/open/244_phase_f1_x86_route6_call_wrapper_diagnostic_oracle_replacement.md`
- x86 Route 6 route-debug tests and helper-oracle tests covering direct-call
  argument source identity
- x86 direct-call wrapper output tests around
  `expected_minimal_direct_extern_call_lane_asm()`
- prepared lookup helper tests covering public `ConsumedPlans` compatibility

## Current Targets

- x86 scalar `i32` direct-call argument source identity path
- Route 6 route-debug rows for positive and fail-closed cases
- helper-oracle statuses for call/argument/source relationship availability
  and failure modes
- prepared fallback path and wrapper assembly stability
- `ConsumedPlans` compatibility visibility

## Non-Goals

- Do not claim broad x86 Route 6 readiness.
- Do not touch riscv call-wrapper readiness or cross-target wrapper convergence.
- Do not delete, hide, privatize, or retire `call_plans`,
  `PreparedFunctionLookups`, `PreparedBirModule`, or draft 155.
- Do not move ABI placement, frame layout, register allocation, stack layout,
  helper/carrier protocol, formatting, instruction selection, emission policy,
  or wrapper policy into BIR ownership.
- Do not use expected-string rewrites, helper renames, unsupported relabeling,
  timeout masking, or baseline refreshes as capability proof.

## Working Model

- Route 6 may become semantic source authority only after agreement with the
  current prepared call argument source selection.
- Fail-closed cases must stay explicit: absent Route 6, invalid source id,
  duplicate Route 6, Route 6/prepared id mismatch, wrong call, wrong argument,
  missing source relationship, missing source value, and source-name mismatch
  must not produce false positive Route 6 wrapper source identity.
- Prepared compatibility remains public and labeled until later ideas prove a
  safe demotion path.

## Execution Rules

- Keep changes behavior-preserving except for explicitly adding route-native
  diagnostic/helper-oracle rows beside compatibility rows.
- Prefer semantic Route 6 fact consumption over fixture-shaped matching.
- Preserve byte-stable route-debug, helper-oracle, fallback, and wrapper output
  where this plan does not explicitly add new route-native visibility.
- Every code-changing step needs fresh build or compile proof plus the narrow
  test subset delegated by the supervisor.
- Before closure, run a matching before/after regression guard over the x86
  route-debug, direct-call wrapper, and prepared lookup helper tests.

## Steps

### Step 1: Establish the x86 Route 6 diagnostic/oracle baseline

Goal: identify the exact x86 scalar `i32` Route 6 source identity path, current
prepared fallback behavior, helper-oracle statuses, and wrapper output contracts
before implementation.

Primary targets:

- x86 Route 6 route-debug tests for direct-call argument source identity
- helper-oracle tests for direct-call call/argument/source relationships
- direct-call wrapper assembly expectations around
  `expected_minimal_direct_extern_call_lane_asm()`
- prepared lookup helper tests for `ConsumedPlans`

Actions:

- Inspect the current x86 Route 6 route-debug rows and map which cases already
  cover positive, absent, invalid, duplicate, mismatch, and source-name mismatch
  behavior.
- Inspect helper-oracle coverage for `available`, `missing_call`,
  `wrong_call`, `missing_argument`, `missing_source_relationship`,
  `missing_source_value`, `abi_bound_excluded`,
  `duplicate_relationship`, and `no_match`.
- Inspect wrapper output tests and prepared fallback expectations that must
  remain unchanged.
- Record gaps in `todo.md` without changing implementation or tests.

Completion check:

- `todo.md` names the exact implementation/test surfaces for the first
  code-changing packet and identifies any missing fail-closed coverage.

### Step 2: Add or tighten route-native fail-closed diagnostic coverage

Goal: make the Route 6 diagnostic/helper-oracle matrix observable before
changing wrapper authority.

Primary targets:

- route-debug coverage for positive, missing, invalid, duplicate, mismatch,
  source-name mismatch, and absent-row behavior
- helper-oracle coverage for applicable failure statuses

Actions:

- Add route-native rows only where the baseline inspection found missing
  observability.
- Keep compatibility rows and expected wrapper assembly unchanged.
- Reject expectation rewrites that only rename or weaken existing prepared
  behavior.

Completion check:

- Narrow x86 route-debug/helper-oracle tests prove the matrix is visible and
  existing compatibility output remains stable.

### Step 3: Introduce agreement-gated Route 6 source authority

Goal: consume Route 6 as the named semantic source authority for the selected
scalar `i32` direct-call argument only when it agrees with prepared source
selection.

Primary targets:

- x86 direct-call argument source identity selection
- Route 6 source fact consumption path
- prepared fallback path

Actions:

- Add the smallest agreement gate needed to compare Route 6 source identity
  against the existing prepared call argument source selection.
- Keep prepared fallback active for missing or non-agreeing Route 6 facts.
- Preserve fail-closed behavior for invalid, duplicate, mismatch, wrong-call,
  wrong-argument, and missing-source cases.

Completion check:

- Positive Route 6 agreement uses Route 6 as the named source authority.
- Non-agreeing or incomplete Route 6 facts do not produce false positives and
  still preserve prepared fallback behavior where currently accepted.

### Step 4: Prove wrapper output and prepared compatibility stability

Goal: verify that route-native authority did not change wrapper assembly,
fallback semantics, or public prepared compatibility status.

Primary targets:

- `expected_minimal_direct_extern_call_lane_asm()`
- direct-call wrapper output tests
- prepared lookup helper tests for `ConsumedPlans`

Actions:

- Run the supervisor-delegated x86 wrapper output subset.
- Run prepared lookup helper proof that `ConsumedPlans` compatibility remains
  public and stable.
- Document any compatibility labels or fallback rows that remain intentionally
  prepared-shaped.

Completion check:

- Wrapper assembly and prepared fallback output remain unchanged except for
  explicitly accepted route-native diagnostic additions.

### Step 5: Run the close-scope regression guard

Goal: prove the completed idea without broadening claims beyond the selected
scalar `i32` x86 path.

Primary targets:

- x86 route-debug tests
- x86 direct-call wrapper tests
- prepared lookup helper tests

Actions:

- Run matching before/after regression guard commands over the close scope.
- Confirm no supported-path downgrade, helper-oracle weakening, wrapper output
  rewrite, or prepared aggregate demotion claim entered the diff.

Completion check:

- Regression guard passes with no new failures.
- The final notes make clear that public prepared aggregates remain retained
  and that riscv/cross-target convergence is still out of scope.
