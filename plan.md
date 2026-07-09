# Pointer/Global Local Publication Authority Runbook

Status: Active
Source Idea: ideas/open/649_pointer_global_local_publication_authority.md

## Purpose

Repair the explicit publication authority path for pointer values derived from
global objects when those pointer values are stored in, loaded from, or
published through ordinary local frame slots.

## Goal

Classify and repair the first `src/pr57861.c` owner only if the prepared facts
prove pointer/global local-publication authority. Otherwise split the row into
a more precise owner and keep this route fail-closed.

## Core Rule

RV64 may consume a pointer-valued local frame-slot access only when pointer
value freshness, local-slot identity, global object identity, and publication
order are explicit. Do not infer pointer/global publication authority from
source spelling, final assembly, diagnostic text, testcase identity, or scalar
frame-slot local-memory facts.

## Read First

- `ideas/open/649_pointer_global_local_publication_authority.md`
- `ideas/closed/640_mixed_local_global_publication_authority.md`
- `ideas/closed/600_pointer_value_memory_use_freshness_authority.md`
- `ideas/closed/631_direct_global_symbol_local_memory_policy.md`

## Current Scope

- Representative surface: `tests/c/external/gcc_torture/src/pr57861.c`.
- Candidate family: pointer value derived from a global object, published
  through an ordinary local frame slot, then loaded or consumed later.
- Expected source example: `short *l = &f`-style pointer/global local
  publication.
- First work is diagnostic classification, not implementation.

## Non-Goals

- Do not reopen scalar frame-slot local-memory lookup repaired by idea 640.
- Do not reopen direct global-symbol local-memory support closed by idea 631.
- Do not reopen generic pointer freshness closed by idea 600 unless this route
  exposes a new local-publication boundary.
- Do not touch aggregate/global-object materialization, stack-home aggregate
  policy, move-bundle fan-in authority, ABI/runtime policy, expectations,
  unsupported markers, allowlists, timeouts, or pass/fail accounting.
- Do not special-case `src/pr57861.c`, local variable names, global names, or
  a final emitted register sequence.

## Working Model

Idea 640 moved scalar frame-slot local-memory lookup forward, leaving
`src/pr57861.c` at a pointer/global local-publication boundary. The active
route must first prove whether the current residual is still that family and
where the owner lives: prepared publication facts, RV64 consumer admission, or
a distinct downstream owner.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Start by refreshing `src/pr57861.c` prepared and RV64 diagnostics after the
  current `main` state.
- Name the prepared pointer value, global object source, local-slot
  destination, load/use point, and publication/freshness authority before any
  implementation packet is selected.
- Add producer or RV64 consumer support only when authority is explicit for the
  selected family.
- Preserve fail-closed diagnostics for stale pointer values, missing global
  object identity, ambiguous publication order, mismatched slots, and
  scalar-only local-memory facts.
- If the refreshed row is not pointer/global local publication, record the
  exact owner and request lifecycle split or park rather than expanding this
  runbook.

## Steps

### Step 1: Refresh Representative Pointer/Global Local Evidence

Goal: Confirm the current first owner for `src/pr57861.c`.

Actions:

- Run focused prepared-BIR, semantic BIR, MIR summary or trace, and RV64
  object/diagnostic probes for `src/pr57861.c`.
- Record the pointer value, global object source, local slot destination,
  load/use point, and current missing or available authority in `todo.md`.
- Distinguish pointer/global local-publication evidence from scalar
  frame-slot local-memory lookup and direct global-symbol local-memory facts.

Completion check:

- `todo.md` states whether the first owner is pointer/global local publication
  and identifies the next implementation or split boundary.

### Step 2: Locate The Publication Authority Boundary

Goal: Find the narrow producer or consumer boundary for the representative
pointer/global local-slot path.

Actions:

- Trace the refreshed prepared facts to the local publication producer and the
  RV64 consumer/admission path.
- Identify whether the missing owner is pointer freshness, global object
  identity, local-slot identity, publication order, or RV64 consumption of an
  already-complete fact.
- Identify a focused backend or prepared/prealloc proof target for a legal
  pointer/global local-publication shape.
- Identify negative behavior for stale pointer values, missing global object
  identity, ambiguous publication order, mismatched local slots, and
  scalar-only facts.

Completion check:

- `todo.md` records the owned implementation surface, focused test target, and
  fail-closed behavior to preserve.

### Step 3: Implement Or Split The Narrow Publication Owner

Goal: Repair one proven pointer/global local-publication owner, or split if the
owner is outside this idea.

Actions:

- If the producer is missing, publish explicit prepared authority only when
  pointer freshness, local-slot identity, global object identity, and
  publication order are proven.
- If the consumer is missing, teach the RV64 path to consume the complete
  authority fact without guessing from source or stack shape.
- Add focused positive coverage for one complete-authority shape.
- Add or preserve negative coverage for scalar-only facts and missing or
  ambiguous pointer/global publication authority.

Completion check:

- Fresh build plus focused proof passes, or lifecycle state records the exact
  split owner and parks this route.

### Step 4: Prove Representative Integration

Goal: Show that `src/pr57861.c` advances through the pointer/global
local-publication failure mode.

Actions:

- Rerun focused coverage plus the representative RV64 route for `src/pr57861.c`.
- Capture evidence showing the pointer/global local-slot access consumes the
  explicit authority, or records a distinct downstream owner.
- Do not claim completion if the row only advances by expectation changes,
  unsupported-marker changes, or testcase-specific handling.

Completion check:

- `todo.md` records representative proof and any remaining downstream owner.

### Step 5: Run Broader Validation And Close Or Park

Goal: Decide whether the source idea is complete after focused and
representative proof.

Actions:

- Run the supervisor-selected broader validation for the affected backend
  bucket after focused proof is green.
- If acceptance criteria are satisfied, request plan-owner close with
  regression-guard proof.
- If a distinct downstream owner remains, record it in `todo.md` and request a
  lifecycle split or park decision.

Completion check:

- Lifecycle state either closes the source idea with passing guard proof or
  records a precise blocked or follow-up owner without broadening this idea.
