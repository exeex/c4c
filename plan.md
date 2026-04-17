# X86 Backend Pure Bir Handoff

Status: Active
Source Idea: ideas/open/13_x86_backend_pure_bir_handoff_and_legacy_guardrails.md

## Purpose

Connect the active x86 backend route to the rebuilt backend pipeline through a
pure BIR or prepared-BIR handoff, then retire the wrong mixed LIR/BIR boundary
instead of preserving it as a fallback.

## Goal

Make the canonical x86 backend route consume backend-side handoff data
directly, delete the mixed transitional route, and prove the pure handoff is
the actual route in use.

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

- x86 backend entry and handoff code under `src/backend/`
- BIR or prepared-BIR handoff surfaces used by the x86 route
- x86-local ownership code that still re-derives stack, ABI, or regalloc facts
- route-proof tests under `tests/backend/`

## Non-Goals

- broad x86 backend completion outside the handoff boundary
- preserving mixed LIR/BIR compatibility theater as a second active path
- rewriting unrelated non-x86 backend families
- proving success with text-shape checks while the real route stays mixed

## Working Model

### Canonical handoff

- x86 should consume canonical backend-side data, either as raw `bir::Module`
  plus explicit local prepare or as an already prepared backend module
- the consumer boundary must be explicit and singular

### Ownership discipline

- canonical backend ABI, location, and frame decisions should flow into x86
- x86 should not maintain a conflicting local shadow contract when the backend
  already publishes the needed facts

### Acceptance model

- runtime proof must show the canonical route is the active route
- deleting or hard-retiring the wrong mixed path is part of completion, not an
  optional cleanup

## Execution Rules

- define the handoff boundary before broad x86 cleanup work
- prefer deleting or hard-retiring the wrong route over preserving it behind a
  compatibility switch
- consume backend-published metadata instead of re-deriving it locally when the
  canonical contract already exists
- keep proof route-oriented and semantic, not based on printed IR substrings
- use `build -> focused x86/backend proof -> broader backend checkpoint` as the
  validation ladder for nontrivial route changes

## Ordered Steps

### Step 1: Lock The X86 Handoff Boundary

Goal:
Decide the canonical x86 consumer boundary and make the wrong mixed boundary
explicitly non-canonical.

Actions:
- inspect the active x86 backend entry path and identify where mixed LIR/BIR
  state is still accepted or re-derived
- choose the honest consumer boundary: raw BIR plus local prepare, or prepared
  backend module
- document the files/functions that currently own x86 handoff selection and the
  seams that must be removed or retired

Completion Check:
- the canonical handoff boundary is explicit in the code and runbook
- the mixed route is identified as deletion/retirement work, not preserved as a
  peer path

### Step 2: Connect X86 To The Canonical Backend Contract

Goal:
Make x86 accept the canonical backend-side handoff instead of the transitional
mixed contract.

Primary Target:
- x86 backend entry and route-selection code under `src/backend/`

Actions:
- connect x86 entry to the chosen pure BIR or prepared-BIR boundary
- remove direct rejection of the canonical handoff when it is not the legacy
  transitional slice
- align x86 consumption to published backend ABI/location metadata where the
  contract already exists

Completion Check:
- the active x86 route accepts the canonical backend-side handoff
- direct route ownership no longer depends on mixed LIR/BIR entry semantics

### Step 3: Retire Mixed Ownership And Legacy Route Behavior

Goal:
Delete or hard-retire the wrong mixed-contract path and remove shadow ownership
that conflicts with canonical backend prepare/prealloc data.

Primary Target:
- mixed handoff branches and x86-local ownership glue under `src/backend/`

Actions:
- remove or hard-retire mixed/fallback x86 entry behavior
- reduce duplicated x86-local stack/regalloc ownership where canonical backend
  metadata already covers the need
- keep any remaining bounded gaps explicit instead of leaving a hidden dual
  path behind

Completion Check:
- the wrong mixed-contract route is no longer an active x86 backend path
- x86 no longer maintains a silent shadow ownership contract for the same data

### Step 4: Runtime Proof And Acceptance

Goal:
Prove the canonical x86 route is the one actually entered and that it consumes
backend-published metadata instead of a hidden mixed fallback.

Actions:
- add or extend focused backend tests that exercise x86 through the canonical
  handoff
- verify route selection and metadata consumption with semantic assertions
- run a broader backend checkpoint before treating the route as close-ready

Completion Check:
- tests prove the canonical x86 route is active
- no hidden mixed fallback is needed for the focused proof
- broader backend validation is green for the accepted route

## Validation

### Focused Proof

- `cmake --build /workspaces/c4c/build --target c4c_backend -j4`
- focused backend/x86 tests under `tests/backend/` that prove route selection
  and metadata use

### Broader Checkpoint

- `ctest --test-dir /workspaces/c4c/build --output-on-failure -R '^backend_'`

## Done Condition

This runbook is complete only when:

- x86 consumes the canonical backend-side handoff
- mixed LIR/BIR ownership is removed or hard-retired for the active route
- canonical backend metadata is used instead of conflicting local re-derivation
- tests prove the canonical x86 route is the one actually in use
