# X86 Backend Pure Bir Handoff

Status: Active
Source Idea: ideas/open/13_x86_backend_pure_bir_handoff_and_legacy_guardrails.md

## Purpose

Connect the active x86 backend route to the rebuilt backend pipeline through a
pure BIR or prepared-BIR handoff, then retire the wrong mixed LIR/BIR boundary
instead of preserving it as a fallback.

## Goal

Keep the canonical x86 backend route on the prepared-module handoff that is now
actually in use, finish retiring any remaining mixed transitional ownership,
and raise validation from focused route proof to a broader backend checkpoint
before treating the route as near-close.

## Core Rule

Do not accept mixed-contract survival as progress.

The run is only complete when the active x86 path both:

- consumes the canonical backend-side handoff
- no longer relies on mixed LIR/BIR ownership or a fallback bridge

## Read First

- `ideas/open/13_x86_backend_pure_bir_handoff_and_legacy_guardrails.md`
- `src/backend/backend.cpp`
- `src/backend/backend.hpp`
- `src/backend/target.cpp`
- x86 backend entry/emission files under `src/backend/`
- tests that currently prove x86 backend route selection and ABI/location use

## Current Targets

- x86 public and direct-BIR entrypoints that now funnel through the
  prepared-module consumer
- remaining mixed ownership or fallback seams under `src/backend/`
- x86-local ownership code that still re-derives stack, ABI, or regalloc facts
- route-proof tests under `tests/backend/`
- broader backend validation needed before near-close acceptance

## Non-Goals

- broad x86 backend completion outside the handoff boundary
- preserving mixed LIR/BIR compatibility theater as a second active path
- rewriting unrelated non-x86 backend families
- proving success with text-shape checks while the real route stays mixed

## Working Model

### Canonical handoff

- the active x86 route now prepares incoming BIR first and consumes one bounded
  prepared-module seam
- public x86 entry and direct x86 BIR entry should continue to converge on that
  singular prepared-module consumer boundary
- further work should treat that prepared-module seam as canonical unless the
  source idea itself changes

### Ownership discipline

- canonical backend ABI, location, and frame decisions should flow into x86
- x86 should not maintain a conflicting local shadow contract when the backend
  already publishes the needed facts

### Acceptance model

- runtime proof must show the canonical route is the active route
- focused prepared/public/generic route-equality proof is necessary but no
  longer sufficient by itself for near-close confidence
- deleting or hard-retiring the wrong mixed path is part of completion, not an
  optional cleanup

## Execution Rules

- define the handoff boundary before broad x86 cleanup work
- prefer deleting or hard-retiring the wrong route over preserving it behind a
  compatibility switch
- consume backend-published metadata instead of re-deriving it locally when the
  canonical contract already exists
- keep proof route-oriented and semantic, not based on printed IR substrings
- keep new work centered on the existing prepared-module consumer boundary, not
  on new public-entry or raw-BIR fallback renderers
- use `build -> focused x86/backend proof -> broader backend checkpoint` as the
  validation ladder, and require the broader `^backend_` checkpoint before
  treating the route as near-close

## Ordered Steps

### Step 1: Checkpoint The Canonical Prepared-Module Boundary

Goal:
Record the prepared-module seam that is now the active x86 route and make the
remaining mixed boundary explicitly residual cleanup, not open route design.

Actions:
- checkpoint the current x86 route where public entry and direct BIR entry
  prepare first and then hand off to the bounded prepared-module consumer
- document that the prepared-module seam is the canonical active boundary for
  this runbook
- identify the remaining mixed ownership or fallback-adjacent seams that still
  need deletion or hard retirement under that boundary

Completion Check:
- the canonical prepared-module handoff boundary is explicit in the runbook
- remaining mixed-route work is framed as cleanup under the chosen boundary,
  not as an unresolved boundary decision

### Step 2: Retire Mixed Ownership Under The Active Boundary

Goal:
Delete or hard-retire the wrong mixed-contract behavior that still survives
around the active prepared-module route.

Primary Target:
- x86 backend entry and route-selection code under `src/backend/`
- x86-local ownership glue that duplicates backend-published prepare/prealloc
  facts

Actions:
- audit the still-reachable mixed or fallback-adjacent x86 entry behavior now
  that the canonical route is the prepared-module consumer
- remove or hard-retire any wrong mixed-contract path that still competes with
  the active prepared-module route
- reduce duplicated x86-local stack, ABI, or location ownership where the
  canonical backend metadata already publishes the needed facts

Completion Check:
- the active x86 route no longer depends on mixed LIR/BIR ownership semantics
- the remaining x86 handoff behavior matches one canonical prepared-module
  route rather than a hidden dual path

### Step 3: Extend Proof Only Where It Serves Route Cleanup

Goal:
Keep focused route proof aligned with the canonical prepared-module consumer
without drifting into test-shaped expansion.

Primary Target:
- route-proof tests under `tests/backend/`

Actions:
- preserve the prepared/public/generic route-equality proof surface for the
  bounded prepared-module consumer
- treat the recently landed joined branch proof as coverage of the current
  bounded family, not as permission to keep widening proof forever
- add more focused proof only when it directly supports the active route and
  stays inside the same explicit consumer seam without fallback growth

Completion Check:
- focused proof still demonstrates that the canonical prepared-module route is
  the path actually in use
- proof expansion remains bounded and semantic rather than testcase-shaped

### Step 4: Broader Backend Checkpoint And Acceptance

Goal:
Raise validation from narrow packet proof to broader backend confidence before
the runbook is treated as close-ready.

Actions:
- keep the focused backend handoff test green for packet-level proof
- run `^backend_` coverage after the next meaningful x86 route slice or before
  any near-close acceptance decision
- use broader validation results to decide whether the route is truly nearing
  completion or whether more mixed-ownership cleanup is still exposed

Completion Check:
- tests prove the canonical x86 route is active
- no hidden mixed fallback is needed for the focused proof
- broader backend validation is green for the accepted route before near-close
  acceptance

## Validation

### Focused Proof

- `cmake --build --preset default -j4`
- `ctest --test-dir build -j --output-on-failure -R '^backend_x86_handoff_boundary$'`

### Broader Checkpoint

- `cmake --build --preset default -j4`
- `ctest --test-dir build -j --output-on-failure -R '^backend_'`

## Done Condition

This runbook is complete only when:

- x86 consumes the canonical backend-side handoff
- mixed LIR/BIR ownership is removed or hard-retired for the active route
- canonical backend metadata is used instead of conflicting local re-derivation
- tests prove the canonical x86 route is the one actually in use
- a broader `^backend_` checkpoint has passed before near-close acceptance
