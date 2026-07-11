# BIR Producer Index View Extraction Runbook

Status: Active
Source Idea: ideas/open/696_bir_producer_index_view_extraction.md

## Purpose

Extract a named BIR producer view for same-block scalar producer identity and
materialization availability without making Route 1 or Route 2 route-numbered
APIs public architecture.

Goal: publish a narrow `BirProducerView`-style surface over existing Route 1
producer facts, include Route 2 select-chain or direct-global facts only when
needed for that producer contract, and migrate one low-risk proof reader
without changing behavior.

## Core Rule

This plan is BIR view extraction only. Route 1 and Route 2 may remain private
compatibility internals behind the named view, but producer lookup must not
become stack destination, publication, value-home, move-bundle, freshness,
prepared, MIR, or target-lowering authority.

## Read First

- `ideas/open/696_bir_producer_index_view_extraction.md`
- `docs/bir_route_index_retirement/research_digest.md`
- `docs/bir_route_index_retirement/ownership_dependencies.md`
- `docs/bir_route_index_retirement/ordered_followup_plan.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets

- BIR producer and select-chain route code under `src/backend/bir/`.
- Existing Route 1 producer identity and materialization lookup surfaces:
  `Route1ProducerIndex`, `Route1SameBlockProducerQuery`,
  `route1_find_same_block_scalar_producer`, and
  `route1_find_materialization_availability`.
- Route 2 select-chain or direct-global facts only if the first named
  producer-view contract requires them.
- Focused BIR/backend tests that prove named producer-view equivalence without
  expectation or harness-policy changes.

## Non-Goals

- Do not extract memory, publication, call, return, dump-policy, or stack
  authority views in this plan.
- Do not migrate prealloc consumers beyond the one proof reader needed to
  prove the named BIR producer view.
- Do not change prealloc executable authority, MIR lowering, target
  materialization, runtime behavior, unsupported markers, allowlists,
  timeouts, or expectations.
- Do not expose `Route1ProducerIndex` or `Route2SelectChainValueIndex` as a
  new public API under different names.

## Working Model

- Route 1 already records same-block producer identity, instruction identity,
  immediate constants, and scalar materialization availability.
- Route 2 already records select-chain producer and direct-global dependency
  facts that may support producer proof for select/control values.
- The named view should own the BIR semantic vocabulary while delegating to the
  existing route builders during migration.
- Missing, unavailable, stale, wrong-type, or wrong-block evidence must remain
  explicit and fail closed rather than falling back to route dumps or testcase
  identity.

## Execution Rules

- Start by choosing one first proof reader and one proof command; record that
  choice in `todo.md`.
- Prefer a thin named view over broad route-builder rewrites.
- Preserve existing record lifetimes, lookup behavior, and negative states.
- Keep Route 2 scoped to producer proof only when the first migration needs
  select-chain or direct-global context.
- If a packet changes executable behavior, stop and escalate to prepared, MIR,
  object, or runtime proof selected by the supervisor.

## Steps

### Step 1: Map The First Producer View Surface

Goal: identify the smallest Route 1 producer lookup or materialization proof
reader that can move through a named BIR producer view.

Primary targets:
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:
- Inspect Route 1 producer index construction, same-block scalar lookup,
  materialization availability lookup, and current focused tests.
- Inspect Route 2 select-chain usage only enough to decide whether the first
  named producer-view contract needs it.
- Choose one low-risk proof reader or focused test surface for migration.
- Record in `todo.md` the chosen consumer, old route entry points, intended
  named view entry point, rollback point, and supervisor proof command.

Completion check:
- `todo.md` names one first producer-view surface and a focused proof command,
  with Route 2 either explicitly included for producer proof or deferred.

### Step 2: Add The Named BIR Producer View

Goal: introduce a named producer-view type or function over existing Route 1
facts without changing behavior.

Primary targets:
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- adjacent BIR proof-view files if the repo already has a better local home

Actions:
- Add a `BirProducerView`-style surface that exposes same-block producer
  identity and materialization availability through named BIR vocabulary.
- Delegate to existing Route 1 route builders and lookup helpers during the
  migration.
- Represent missing or unavailable producer evidence explicitly.
- Keep `Route1ProducerIndex` private compatibility behind the named view.

Completion check:
- The named view compiles and returns the same producer identity and
  materialization availability as the old Route 1 path for the selected
  surface.

### Step 3: Scope Route 2 Select-Chain Support If Needed

Goal: include Route 2 select-chain or direct-global facts only when they are
required by the selected producer-view proof.

Primary targets:
- `Route2SelectChainValueIndex`
- `route2_build_select_chain_value_index`
- `route2_find_select_chain_value_record`

Actions:
- If Step 1 selected a Route 1-only producer surface, leave Route 2 private and
  untouched except for any compile wiring required by the named view.
- If Step 1 requires select-chain or direct-global evidence, add a narrow
  named producer-view query for that evidence without exposing Route 2 as
  public architecture.
- Preserve missing, unavailable, wrong-key, stale, and unsupported states.

Completion check:
- Route 2 is either explicitly deferred in `todo.md` or covered by a narrow
  named producer-view proof that remains private compatibility internally.

### Step 4: Migrate One Proof Reader

Goal: move the selected proof reader from route-numbered lookup to named BIR
producer-view vocabulary.

Actions:
- Replace the selected direct Route 1 or Route 2 public proof read with the
  named producer-view API.
- Keep rollback to the old route API local and straightforward.
- Do not mix this migration with memory, publication, call, return, prealloc
  authority, target lowering, or dump cleanup.
- Add or adjust focused tests only when they prove the named producer-view
  contract without weakening an existing expectation.

Completion check:
- One proof reader uses named producer-view vocabulary and behavior remains
  equivalent to the old route-numbered path.

### Step 5: Prove Behavior Preservation

Goal: produce fresh proof that the named producer view is equivalent for the
selected surface and does not expand route-numbered public architecture.

Actions:
- Run `cmake --build --preset default`.
- Run the focused BIR/backend proof command selected in Step 1.
- If the diff affects shared backend behavior beyond the focused proof, ask
  the supervisor to run a matching broader backend guard.
- Inspect the diff for public expansion of Route 1/Route 2 APIs, expectation
  rewrites, unsupported downgrades, allowlist edits, timeout edits, or
  route-dump-only proof.

Completion check:
- Build and focused proof pass.
- Route 1 and Route 2 remain private compatibility internals.
- Missing producer evidence remains explicit and fail-closed.
