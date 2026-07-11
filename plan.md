# Prepared Fact Boundary From Named BIR Views Runbook

Status: Active
Source Idea: ideas/open/705_prepared_fact_boundary_from_bir_views.md

## Purpose

Move executable prepared decisions behind a narrow prealloc-owned fact
boundary that consumes the named BIR semantic views completed by idea 704.

## Goal

Migrate prepared publication, call-plan, and lookup attribution production to
named BIR inputs while preserving existing prepared homes, moves, freshness,
frame, and control authority and removing route records from public prepared
records.

## Core Rule

Named BIR views are source-semantic input evidence only. Prealloc must uniquely
select executable prepared facts, report incomplete or ambiguous input
explicitly, and never promote route status or route agreement into authority.

## Read First

- `ideas/open/705_prepared_fact_boundary_from_bir_views.md`
- `docs/bir_mir_contract_abstraction/02_ownership_and_named_handoff_contracts.md`
- `docs/bir_mir_contract_abstraction/04_handoff_audit_and_closure_evidence.md`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/module.hpp`

## Current Scope

- Prepared block-entry, current-block, edge, formal, and store-source
  publication production.
- Prepared call plans and argument/result attribution.
- Prepared lookup attribution and the existing home, frame, move, freshness,
  control, and publication facts it connects.
- Public prepared record headers, printers, and contract tests affected by
  those migrated producer seams.
- Explicit missing, incomplete, ambiguous, mismatched, and unsupported input
  states at the prepared boundary.

## Non-Goals

- Do not migrate the common MIR query layer owned by idea 706.
- Do not implement the positive stack-destination authority gate owned by idea
  707.
- Do not migrate x86, AArch64, or RV64 target materializers.
- Do not delete private BIR route builders needed behind named views.
- Do not create parallel frame, home, move, freshness, publication, call-plan,
  lookup, or control authority merely to adopt a proposed contract name.
- Do not expose a route record, route index, or route-numbered authority field
  through a public prepared record.
- Do not weaken expectations, unsupported markers, or proof breadth.

## Working Model

- Named BIR queries identify producer, memory-access, publication, call, and
  control relationships with explicit availability.
- A named BIR publication query is required only when the BIR program contains
  the source-semantic publication relationship being consumed.  A publication
  created by prealloc from an authoritative prepared `JoinTransfer` is a
  distinct prepared-originated family, not a missing BIR publication and not a
  route-fallback case.
- Prealloc combines those relationships with existing prepared homes, frame
  layout, moves, freshness, publication, call-plan, and control state.
- A prepared fact is positive only when required inputs are present,
  consistent, and uniquely selected; every other state remains explicit and
  fail closed.
- Route-backed agreement may remain temporarily in private observational proof
  but cannot select a fact or survive in a public prepared payload.

## Execution Rules

- Migrate one producer seam at a time and keep its positive and negative proof
  in the same step.
- Adapt existing prepared facts where they already own the decision; do not
  duplicate them under speculative new view types.
- Bind facts to stable function, block, instruction, edge, value, or call
  identity as appropriate and reject mismatches.
- Classify each producer seam by semantic origin before choosing its required
  named inputs.  Never make a non-applicable named view mandatory merely to
  prove that prepared-owned state exists.
- Keep compatibility route access private and shrinking; do not add new route
  callers while migrating a seam.
- For each code-changing step, run the supervisor-delegated build and focused
  prepared/prealloc tests before acceptance.
- Run the umbrella route-vocabulary guard after public-header changes and use
  broader backend validation at the final integration step.

## Ordered Steps

### Step 1: Establish the prepared boundary inventory and status contract

Goal: identify the executable prepared records that still expose or select
route-shaped inputs and define the narrow fail-closed boundary for migration.

Primary targets:

- `src/backend/prealloc/module.hpp`
- public prepared headers under `src/backend/prealloc/`
- prepared contract and printer tests under `tests/backend/bir/`

Actions:

- Inventory route types, route indexes, route-numbered fields, and agreement
  branches in publication, call-plan, lookup, home, and control records.
- Classify every hit as executable authority, public payload, private
  observational compatibility, debug/proof, or deferred downstream consumer.
- Reuse or narrowly extend prepared availability/status vocabulary so missing,
  incomplete, ambiguous, mismatched, and unsupported named BIR input remain
  distinguishable.
- Establish a focused public-record/contract guard that prevents route records
  and indexes from crossing the prepared boundary.

Completion check:

- The migration inventory names the first owned producer seams, public
  prepared records reject route-shaped payloads, and focused proof covers a
  positive prepared fact plus explicit negative input without inventing new
  authority.

### Step 2: Migrate prepared publication production

Goal: make prepared publication plans consume named BIR publication, producer,
memory, and control facts where those source-semantic relationships exist,
while prealloc remains the sole owner of executable publication decisions and
prepared-originated `JoinTransfer` publications.

Primary targets:

- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/formal_publications.cpp`

Actions:

- Inventory the remaining publication seams by semantic origin before the next
  migration: BIR source-semantic publication, prepared-originated
  `JoinTransfer`, formal, or store-source.  Record which named BIR facts are
  applicable to each family and which prepared facts authorize the executable
  publication.
- Replace direct Route 4/5 publication record and index inputs at the migrated
  BIR source-semantic seams with stable named BIR view queries or narrow facts.
- For a valid non-PHI prepared `JoinTransfer` edge publication, require the
  unique cursor/edge, source/destination value, home, move, freshness, carrier,
  and control facts already owned by the prepared producer.  Use applicable
  named BIR producer or control evidence to bind source identities, but do not
  require or synthesize a BIR CFG-edge publication relation that does not exist
  in the BIR program.
- Make evidence applicability explicit before selection.  If a named BIR fact
  is required for the classified family, missing, incomplete, ambiguous, or
  mismatched evidence fails closed.  If the fact is non-applicable to a
  prepared-originated family, proceed only through the independently
  authoritative prepared path; do not reinterpret `MissingPublication` as
  success and do not consult a route fallback.
- Preserve prepared source/destination homes, selected moves, freshness,
  cursor/edge identity, and publication status as prealloc-owned decisions.
- Keep any route agreement as private observational proof only, then remove it
  from executable selection and public publication payloads.
- Add focused block-entry, current-block, edge, formal, and store-source proof
  for available and missing/incomplete/ambiguous/mismatched applicable inputs.
  Include a non-PHI prepared `JoinTransfer` edge case such as
  `dispatch.edge.add` proving that non-applicable CFG-edge publication evidence
  neither rejects valid prepared behavior nor becomes an implicit fallback.

Completion check:

- Migrated BIR-origin publication plans are selected solely from applicable
  named BIR evidence plus prepared authority; prepared-originated
  `JoinTransfer` publications are selected solely from complete, unique
  prepared authority plus any applicable named producer/control identity;
  public records carry no route-numbered executable state; and focused
  publication tests remain green without route fallback, synthesized BIR
  publication relations, or weakened supported cases.

### Step 3: Migrate prepared call-plan production

Goal: use named BIR call-boundary and producer facts to build existing prepared
call plans without granting BIR ABI placement authority.

Primary targets:

- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/call_plans.hpp`

Actions:

- Replace direct Route 6 source/result attribution at migrated seams with
  `BirCallBoundaryView` and the required narrow producer facts.
- Keep argument/result homes, lanes, ABI resources, moves, and materialization
  choices owned by the prepared call-plan producer.
- Reject unavailable, incomplete, ambiguous, mismatched-call, and mismatched-
  value inputs without route discovery fallback.
- Add focused direct-call, argument-source, result, and negative-state proof.

Completion check:

- Call plans consume named call relationships, retain prealloc-owned ABI and
  placement decisions, and expose no route record/index in their public
  contract.

### Step 4: Migrate prepared lookup attribution

Goal: make prepared lookup and block-entry attribution consume named BIR facts
without reconstructing semantic routes or weakening home selection.

Primary targets:

- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/lookup_agreement.cpp`
- `src/backend/prealloc/value_locations.hpp`

Actions:

- Replace direct route-index and Route 4 attribution inputs with named BIR
  publication, producer, memory, or control facts according to ownership.
- Preserve value ids, homes, frame slots, lookup cursor, publication linkage,
  and freshness selection in existing prepared records.
- Remove route-numbered attribution/status fields from public records once the
  named producer seam supplies equivalent narrow evidence.
- Prove available lookup attribution and missing, ambiguous, stale, and
  identity-mismatched rejection.

Completion check:

- Prepared lookups are cursor- and identity-bound, fail closed without unique
  named input, and neither public lookup nor value-location records expose a
  route record or route index.

### Step 5: Align prepared records, printers, and store-source proof

Goal: finish the public prepared surface for the migrated producers and ensure
debug output reports ownership-named evidence rather than executable route
authority.

Primary targets:

- prepared record headers under `src/backend/prealloc/`
- `src/backend/prealloc/prepared_printer.cpp` and its helpers
- prepared contract, printer, and store-source tests

Actions:

- Remove migrated route-numbered fields from public prepared records and their
  printers; retain private compatibility only where an unmigrated consumer
  still requires it.
- Report prepared availability, attribution, selected authority, and rejection
  reason with ownership-named vocabulary.
- Verify store-source publication uses the migrated producer and lookup seams
  while preserving selected home and freshness authority.
- Strengthen focused tests so direct route fixtures are not the sole proof of
  any migrated prepared fact.

Completion check:

- Public prepared headers and migrated printer output contain no route-shaped
  executable contract, and contract/printer/store-source tests prove both
  positive production and fail-closed negative states.

### Step 6: Audit the boundary and run integration proof

Goal: prove idea 705's prealloc-owned boundary is complete without absorbing
common MIR, target, or stack-destination work.

Actions:

- Run the umbrella route-vocabulary guard and classify every remaining
  prealloc hit as private observational compatibility, deferred scope, or a
  violation; the guarded public-record and executable-selection set must
  shrink.
- Verify migrated prepared selection never branches on route status/agreement
  and public records do not embed route records or route-numbered authority.
- Verify no parallel authority was introduced and negative named BIR inputs
  remain explicit and fail closed.
- Run the full focused prepared publication, call-plan, lookup, printer, and
  store-source set plus the supervisor-selected broader backend regression
  check.
- Record common MIR migration as idea 706 scope and the positive stack-
  destination producer gate as idea 707 scope.

Completion check:

- Prepared publication, call-plan, and lookup attribution consume named BIR
  facts; executable homes, moves, freshness, frame, publication, and control
  remain prealloc-owned; the retirement guard has shrunk; and broader proof is
  green without route fallback or weakened tests.
