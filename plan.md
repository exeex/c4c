# X86 Defined-Function Prepared-Core Completion Runbook

Status: Active
Source Idea: ideas/open/721_x86_defined_function_prepared_core_completion.md

## Purpose

Repair the common preparation contract so every eligible defined function
reaches x86 module emission with a genuine prepared-core function view.

## Goal

Localize and repair the earliest missing prepared-core production boundary
without weakening the x86 emitter invariant or synthesizing target-side facts.

## Core Rule

Prepared-core readiness must be produced by the general preparation pipeline.
Do not bypass, catch, or weaken the x86 emission check, and do not special-case
the known fixture or function.

## Read First

- `ideas/open/721_x86_defined_function_prepared_core_completion.md`
- the failing `backend_prepare_frame_stack_call_contract` fixture and its
  production path
- common prepared-core function-view production and lookup code
- x86 module emission's prepared-core readiness check

## Scope

- Identify the exact defined function missing its prepared-core view.
- Trace the function from preparation eligibility through common publication
  and x86 lookup.
- Repair the earliest general producer/admission boundary that drops or omits
  the view.
- Add focused positive coverage for the failing shape and a nearby defined
  function shape.
- Preserve precise failure for genuinely absent or inconsistent preparation.

## Non-Goals

- Do not change block-entry publication identity or proof attribution.
- Do not expand prepared call-plan cursor semantics unless localization proves
  that contract is the direct cause and lifecycle review approves the route.
- Do not treat definitions as declarations or inject readiness in fixtures.
- Do not change ABI policy, register spelling, move ordering, or unrelated
  target lowering.
- Do not weaken expectations, unsupported classifications, or the emitter
  invariant.

## Working Model

The x86 emitter is a fail-closed consumer of a common prepared-core function
view. The repair belongs at the earliest general producer boundary where an
eligible defined function is omitted, rejected, or loses identity. Target-side
fallback is not producer authority.

## Execution Rules

- Work in ordered, reviewable steps and update `todo.md` after each packet.
- Record the missing function, its identities, and the first bad fact before
  implementation.
- Prefer semantic identity and general eligibility rules over fixture names,
  function names, or testcase shape.
- Keep absent, duplicate, stale, mismatched, and inconsistent preparation
  fail closed with typed or precise diagnostics.
- For code-changing steps, run the supervisor-delegated build and focused test
  command exactly; broader x86 or backend proof is required before closure.
- Stop for lifecycle review if the first bad fact belongs specifically to the
  prepared call-plan cursor contract or requires expansion into idea 718 scope.

## Step 1: Localize The Missing Prepared-Core Fact

Goal: identify the exact defined function and earliest producer boundary where
its common prepared-core view is lost.

Primary targets:

- `backend_prepare_frame_stack_call_contract` production path
- common preparation function eligibility, publication, and lookup
- x86 module prepared-core lookup and invariant

Actions:

- Reproduce the focused failure without changing expectations.
- Record the failing function's stable identities and definition status.
- Trace whether the view is never produced, rejected during admission, stored
  under mismatched identity, or unavailable during x86 lookup.
- Compare with at least one nearby defined function that successfully receives
  a prepared-core view.
- Name the earliest owning producer seam and confirm it is within idea 721.

Completion check:

- `todo.md` records the exact function, first bad fact, successful comparison,
  owning seam, and focused reproduction command.
- No implementation or fixture workaround is claimed as localization.

## Step 2: Repair General Prepared-Core Production

Goal: make every eligible defined function publish and retain the genuine
common prepared-core view.

Actions:

- Implement the smallest general repair at the Step 1 producer/admission seam.
- Preserve stable function identity from production through lookup.
- Preserve precise rejection for absent, inconsistent, duplicate, stale, or
  mismatched preparation.
- Do not add x86 fallback, fixture injection, or named-function conditions.

Completion check:

- The localized failing function and the nearby comparison shape both reach
  x86 emission through the common prepared-core view.
- Focused build and delegated tests pass without expectation changes.

## Step 3: Strengthen Focused Contract Coverage

Goal: prove the repaired contract across positive and fail-closed shapes.

Actions:

- Add or extend focused coverage for the original function shape.
- Cover at least one nearby eligible defined-function shape.
- Cover genuinely absent or inconsistent preparation and verify that x86
  emission still fails precisely.
- Inspect the diff for fixture-only readiness injection, invariant weakening,
  or testcase-shaped branching.

Completion check:

- Positive coverage proves genuine common production for multiple shapes.
- Negative coverage proves missing or inconsistent readiness remains closed.
- The supervisor-selected focused proof is green.

## Step 4: Validate The Broader X86/Backend Contract

Goal: establish that the producer repair is safe beyond the focused fixture.

Actions:

- Run the supervisor-selected broader x86 or backend comparison using matching
  before/after commands and canonical regression logs.
- Investigate any changed failure ownership rather than weakening tests.
- Confirm no idea 718 attribution, idea 716 cursor semantics, ABI policy, or
  unrelated target lowering changed without lifecycle approval.

Completion check:

- Focused and broader proof are green with no expectation downgrade.
- Every acceptance criterion and reviewer reject signal in the source idea has
  been checked explicitly before requesting lifecycle closure.
