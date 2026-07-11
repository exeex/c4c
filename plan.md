# Route Fact Test And Dump Contract Cleanup Runbook

Status: Active
Source Idea: ideas/open/701_route_fact_test_dump_contract_cleanup.md

## Purpose

Retire or gate one transitional route-numbered dump/test vocabulary family only
after the matching named BIR, prepared, MIR, object, object-runtime, or runtime
proof surface already exists.

## Goal

Rewrite one Route 4, Route 5, or Route 7 dump/test vocabulary family to named
publication, join-source, edge-publication, or comparison agreement vocabulary
without weakening executable proof.

## Core Rule

Named proof must lead dump cleanup. Do not create route dump spelling that
claims a semantic migration before the named proof surface exists.

## Read First

- `ideas/open/701_route_fact_test_dump_contract_cleanup.md`
- `docs/bir_route_index_retirement/research_digest.md`
- `docs/bir_route_index_retirement/ownership_dependencies.md`
- `docs/bir_route_index_retirement/ordered_followup_plan.md`

## Current Scope

- Backend tests, dump expectations, and diagnostic printers that own route fact
  dump vocabulary.
- One Route 4, Route 5, or Route 7 route-numbered vocabulary family.
- Cleanup tied to an existing named proof surface, not new executable
  semantics.

## Non-Goals

- Do not create named BIR view semantics as part of this cleanup.
- Do not migrate prealloc consumers.
- Do not change executable prepared, MIR, object, or runtime behavior.
- Do not weaken expectations, unsupported markers, allowlists, default harness
  contracts, timeout policy, or pass/fail accounting.
- Do not keep the same route-only assertion under a renamed expected-output
  file and claim progress.

## Working Model

- Route 4 transitional dump labels may remain private compatibility until a
  named block-entry or current-block publication proof exists for the row being
  renamed.
- Route 5 `route5_status`, `route5_agrees`, and `route5_join_source` may remain
  private compatibility until named current-block join-source or
  edge-publication proof exists.
- Route 7 route-index status labels may remain private compatibility until
  named comparison agreement proof exists.
- Stronger executable proof should replace route dump proof when runtime,
  object-runtime, object, MIR, prepared-MIR, or prepared contract coverage is
  available.

## Execution Rules

- Move exactly one vocabulary family per implementation slice.
- Start from the existing named proof surface; if the surface is absent, record
  the blocker in `todo.md` and stop rather than inventing a dump rename.
- Preserve old route-numbered builders, fields, or labels as private
  compatibility when consumers still need them.
- Keep proof at least as strong as the old route dump contract.
- Treat expectation rewrites, unsupported-marker changes, allowlist edits, and
  baseline-only acceptance as route drift, not cleanup.
- For code-changing steps, use the supervisor-delegated proof command exactly.

## Ordered Steps

### Step 1: Select an eligible vocabulary family

Goal: identify one Route 4, Route 5, or Route 7 dump/test vocabulary family
whose named proof surface already exists.

Primary targets:

- Backend test expectations and CMake wiring that mention transitional route
  vocabulary.
- Diagnostic printers that emit route proof vocabulary.
- Existing prepared, MIR, object, object-runtime, runtime, or named BIR
  route-view proof for the candidate row family.

Actions:

- Inspect route-numbered dump/test vocabulary for Route 4, Route 5, and Route
  7.
- For each candidate, identify the stronger or named proof surface that already
  proves the same behavior.
- Choose one smallest candidate where cleanup follows that proof surface.
- If no candidate has an existing named proof surface, leave the route spelling
  untouched and record the blocker.

Completion check:

- `todo.md` names the selected vocabulary family, the existing proof surface,
  the files expected to change, and the proof command the supervisor delegated;
  or it records that no eligible candidate exists.

### Step 2: Rewrite or gate the selected route vocabulary

Goal: remove, rename, or gate the selected transitional route-numbered dump/test
vocabulary behind its named proof surface.

Primary targets:

- The selected test expectation files, diagnostic printer rows, and any narrow
  CMake/test wiring needed for that family.

Actions:

- Replace route-numbered public-facing spelling with the named proof vocabulary
  for the selected family.
- Keep route-numbered internals only as private compatibility where still
  required.
- Remove default intermediate dump coupling when a stronger connected proof
  already covers the behavior.
- Avoid unrelated route families and broad test policy churn.

Completion check:

- The selected route vocabulary no longer appears as the public/default harness
  contract for the chosen row family.
- Compatibility route labels remain only where still needed and are not treated
  as executable authority.

### Step 3: Prove the cleanup did not weaken coverage

Goal: validate that the renamed or gated vocabulary is backed by proof at least
as strong as the previous route dump contract.

Actions:

- Run the supervisor-delegated build and narrow backend proof command.
- If executable behavior is implicated, escalate to the stronger connected
  proof surface identified in Step 1.
- Record exact proof commands and outcomes in `todo.md`.

Completion check:

- Fresh proof is green, or blockers identify the missing named proof surface or
  coverage gap without weakening expectations.

### Step 4: Decide whether idea 701 is complete

Goal: determine whether the source idea is satisfied after one vocabulary family
cleanup or whether another route-numbered dump family remains in scope.

Actions:

- Compare the completed cleanup against the source idea acceptance criteria.
- Leave durable remaining-route notes in `todo.md` for the supervisor if more
  cleanup should become a follow-up packet.
- Do not close merely because the current runbook is exhausted; closure belongs
  to plan-owner after supervisor review and regression guard.

Completion check:

- The supervisor has enough state in `todo.md` to delegate another packet or ask
  plan-owner to close, park, or split the lifecycle state.
