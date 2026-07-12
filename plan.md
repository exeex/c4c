# RV64 Named Handoff Materializer Cleanup Runbook

Status: Active
Source Idea: ideas/open/710_rv64_named_handoff_materializer_cleanup.md
Activated after: closure of idea 709

## Purpose

Retire executable Route 3/5 agreement and route-labelled intent from RV64 MIR
materialization now that prepared publication and common named queries are
available.

## Goal

Make RV64 edge publication and object emission consume prepared ownership-named
authority exclusively while preserving target instruction and ABI behavior.

## Core Rule

Prepared publication is the sole executable source and move authority. Missing
or inconsistent prepared input must fail closed; route agreement, dumps, and
final assembly are never substitute authority.

## Read First

- `ideas/open/710_rv64_named_handoff_materializer_cleanup.md`
- `ideas/closed/705_prepared_fact_boundary_from_bir_views.md`
- `ideas/closed/706_common_mir_named_query_migration.md`
- `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`
- directly implicated prepared publication and RV64 object-intent tests

## Current Scope

- RV64 prepared edge-publication emission.
- Directly implicated RV64 object-intent construction and callers.
- Focused prepared publication and object-emission behavior/dump proof.
- The semantic RV64 route-vocabulary retirement guard.

## Non-Goals

- No x86, AArch64, BIR-internal, or common producer/query redesign.
- No ABI, register-allocation, scheduling, or unrelated instruction-selection
  policy changes.
- No expectation weakening, unsupported reclassification, fixture-only
  authority injection, or testcase-shaped fallback.
- Debug-only route text may be deferred to idea 712, but it must not affect
  emission.

## Working Model

- Common preparation owns source identity, move identity, publication,
  freshness, and availability.
- RV64 validates and realizes that prepared intent into target instructions and
  object intent.
- Route 3/5 agreement rows are legacy observations, not executable inputs.

## Execution Rules

- Inspect and classify before editing; distinguish executable route dependency
  from debug-only vocabulary.
- Replace each executable route decision with an existing typed prepared view,
  including owner and stable-identity validation.
- Stop for lifecycle review if a required semantic fact is not published by an
  existing common contract; do not reconstruct it in RV64.
- Delete obsolete agreement/index/fallback code after its final semantic
  consumer disappears; do not hide it behind a renamed helper.
- For every code-changing step run the supervisor-selected build and focused
  test command. Require a broader RV64/backend comparison before completion.

## Ordered Steps

### Step 1: Audit RV64 executable route authority

Goal: establish the exact semantic Route 3/5 and agreement dependencies that
remain in RV64 emission and identify their prepared replacements.

Primary target:
`src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp` and its
direct callers and tests.

Actions:

- Trace each route/agreement read through edge-publication and object-intent
  construction.
- Classify each hit as executable authority, compatibility fallback,
  diagnostic-only text, or dead residue.
- Map every executable read to an existing ownership-named prepared query and
  record required owner, source, move, publication, and freshness identities.
- Identify focused positive and missing/inconsistent-input negative tests.
- Stop and request lifecycle review if any required replacement contract is
  absent from common preparation.

Completion check:

- Every scoped executable route dependency has a concrete existing prepared
  replacement and focused proof surface, or the first missing common contract
  is documented as a lifecycle blocker before implementation.

### Step 2: Migrate prepared edge-publication emission

Goal: remove Route 3/5 agreement and compatibility fallback from RV64 prepared
edge-publication emission.

Actions:

- Consume the typed prepared publication source and move authority identified
  in Step 1.
- Validate ownership and stable identities before target realization.
- Preserve supported register, immediate, and memory publication behavior
  represented by the existing contract.
- Make missing, stale, ambiguous, mismatched, unsupported, or incomplete
  prepared input fail closed with precise state.
- Delete the replaced route/agreement branch and fallback after its final
  consumer is gone.
- Run the delegated build and focused RV64 prepared-publication proof without
  expectation changes.

Completion check:

- Edge-publication emission has no executable route-agreement dependency,
  supported focused behavior is green, and negative prepared states fail
  closed.

### Step 3: Migrate RV64 object intent

Goal: ensure object intent derives semantic decisions only from
ownership-named prepared facts.

Actions:

- Replace directly implicated route-labelled executable intent with prepared
  owner/source/move facts.
- Retain only RV64-local instruction, relocation, scheduling, and ABI
  realization.
- Remove obsolete route-derived builders, indexes, or compatibility adapters
  after their final consumer disappears.
- Prove object behavior and structured dumps independently of agreement rows;
  do not use final assembly alone as authority proof.
- Run the delegated build and focused object-emission proof.

Completion check:

- RV64 object intent uses ownership-named prepared facts exclusively, focused
  behavior/dump proof is green, and no target-local semantic reconstruction
  remains.

### Step 4: Prove semantic retirement and disposition

Goal: demonstrate zero route-labelled executable authority in semantic RV64
emission and obtain broader regression confidence.

Actions:

- Search semantic RV64 emission for Route 3/5, agreement, route index, and
  renamed reconstruction/fallback patterns.
- Remove local dead/debug residue when safe or classify non-semantic debug text
  explicitly for idea 712.
- Run the supervisor-selected broader RV64/backend matching regression
  comparison.
- Review the complete idea 710 diff against its acceptance criteria and reject
  signals.

Completion check:

- The semantic RV64 retirement guard is zero, broader proof has no new
  failures, and no route agreement, renamed executable dependency, expectation
  weakening, or final-assembly-only proof remains.
