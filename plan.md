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
- The bounded AArch64 compatibility adapter needed to consume or transport an
  already prealloc-owned store-source publication fact after its producer
  contract changes; this does not make the target an owner of that fact.

## Non-Goals

- Do not migrate the common MIR query layer owned by idea 706.
- Do not implement the positive stack-destination authority gate owned by idea
  707.
- Do not migrate x86, AArch64, or RV64 target materializers.  Step 2.1 may
  narrowly adapt the existing AArch64 store-local compatibility seam only to
  consume the unique precomputed prealloc publication record or, if that
  record cannot be reached without broader migration, transport the named
  producer evidence and block identity already available to its lowering
  context into the common prealloc planner.
- Do not let the Step 2.1 AArch64 adapter replan publication semantics, select
  executable publication authority, synthesize named evidence, or add
  testcase- or producer-shape exceptions.
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

#### Step 2.1: Close and inventory the store-source boundary

Goal: close the remaining named-producer applicability hole and establish the
complete residual store-source producer-family inventory before migrating
another publication family.

Actions:

- Derive cast/select named producer evidence applicability from the producer
  family, not from the presence of `source_producer_block_label`.
- Treat a missing block label for a cast or select as incomplete required
  identity evidence and fail closed; add focused negative proof for both
  families.
- Inventory every residual store-source producer family and record which named
  BIR producer, memory, publication, or control facts apply and which prepared
  home, access, freshness, and ordering facts authorize selection.
- Confirm the normal population, pending-global, direct-global, and fixed-
  formal callers all pass through the same fail-closed store-source selection
  boundary without route discovery fallback.
- Adapt the existing AArch64 store-local compatibility caller as one bounded
  packet. Prefer lookup and consumption of the unique store-source publication
  record already produced by prealloc. If that record is not addressable at
  this seam without migrating the target materializer, narrowly transport the
  named producer result and its BIR block-label identity already available in
  `BlockLoweringContext` to the common prealloc planner instead.
- Keep all executable publication selection in prealloc. The adapter may only
  consume or transport prealloc inputs/results; it must not derive target-side
  authority, fabricate identity evidence, dispatch on a named testcase or
  producer shape, weaken a supported expectation, or broaden into target
  materializer migration.

Completion check:

- Cast and select cannot disable required named producer evidence by omitting
  the block label; every residual store-source family has an explicit evidence
  applicability classification; the supported AArch64 compatibility paths
  consume or transport the same prealloc-owned decision without target-side
  authority; and focused positive plus missing, incomplete, ambiguous, and
  mismatched proof and the supervisor-delegated backend proof are green.

#### Step 2.2: Migrate formal publication production

Goal: make formal publication consume the applicable named BIR evidence while
retaining prepared home, move, freshness, frame, and publication authority.

Actions:

- Inventory formal publication origins and distinguish BIR source-semantic
  relationships from prepared composition, including fixed-formal
  store-source composition.
- Replace direct Route 4/5 record or index inputs at migrated BIR-origin formal
  seams with stable named BIR view queries or narrow facts.
- Require complete, unique, identity-matched applicable evidence and reject
  missing, incomplete, ambiguous, or mismatched input without a route fallback.
- Preserve formal destination, selected home, frame, move, freshness, and
  publication status as prealloc-owned decisions.
- Add focused formal positive and negative proof, including composition with
  the Step 2.1 store-source boundary.

Completion check:

- Formal publication selection uses only applicable named BIR input plus
  prepared authority, carries no route-numbered executable authority in its
  migrated public payload, and its focused positive and fail-closed negative
  proof is green.

#### Step 2.3a: Contract publication selection at the prealloc boundary

Goal: make block-entry, edge, and current-block publication selection depend
only on applicable named BIR evidence plus independently authoritative prepared
state before changing compatibility consumers.

Primary targets:

- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- directly affected prepared publication tests under `tests/backend/bir/`

Actions:

- Replace remaining direct Route 4/5 publication record and index inputs at
  BIR source-semantic seams with stable named BIR view queries or narrow facts.
- Classify evidence applicability before selection. Missing, incomplete,
  ambiguous, or mismatched applicable evidence must fail closed without a route
  fallback.
- For a non-PHI prepared `JoinTransfer`, require the unique cursor/edge,
  source/destination value, home, move, freshness, carrier, and control facts
  already owned by prealloc. Use applicable named producer/control evidence to
  bind source identity, but do not require or synthesize a BIR CFG-edge
  publication relation that the BIR program does not contain.
- Preserve the existing Route 5 compatibility payload only until Step 2.3b;
  prove in this step that it no longer participates in prealloc selection.
- Add focused positive and missing/incomplete/ambiguous/mismatched proof,
  including the non-PHI `dispatch.edge.add` prepared `JoinTransfer` case.

Completion check:

- Prealloc publication selection branches only on applicable named BIR
  evidence plus complete prepared authority; Route 5 compatibility fields are
  observational only; `dispatch.edge.add` remains available without synthesized
  BIR edge-publication evidence; and focused publication proof is green.

#### Step 2.3b: Retire the bounded Route 5 compatibility consumer

Goal: remove the route-numbered public publication payload and migrate only its
known compatibility consumers to the Step 2.3a named/prepared contract.

Primary targets:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/prealloc/prepared_printer/select_chains.cpp`
- directly affected prepared and AArch64 contract tests

Actions:

- Remove `route5_join_source`, `route5_join_source_status`,
  `route5_join_source_agrees`, and the Route 5 index input from the public
  current-block publication fact/query payload.
- Replace the AArch64 dispatch predicate that reads those fields with the
  already-produced named evidence and prepared publication, home, move,
  freshness, and edge identity. Do not replan publication semantics in the
  target and do not migrate unrelated target materializers.
- Remove Route 5 construction and transport used only by this prepared query.
- Update the prepared printer and the directly affected contract test to expose
  and assert the named/prepared boundary rather than route status/agreement.
- Reject any widening into common MIR query migration, other AArch64 producer
  families, x86/RV64 materializers, or target-owned publication authority.

Completion check:

- Public prepared publication records and query inputs expose no route-numbered
  executable state; the bounded AArch64 consumer uses the Step 2.3a authority
  without selecting new semantics; printer/tests contain no stored Route 5
  agreement contract; and focused prepared plus AArch64 proof is green.

#### Step 2.3c: Integrate and prove publication-family closure

Goal: verify the completed block-entry, edge, and current-block contraction as
one coherent publication-family boundary.

Actions:

- Audit migrated public publication payloads and selection branches for Route
  4/5 records, statuses, indexes, agreement predicates, or fallbacks.
- Re-run focused available and missing/incomplete/ambiguous/mismatched proof for
  block-entry, current-block, and edge families, including
  `dispatch.edge.add`.
- Run the supervisor-selected broader backend proof and preserve its canonical
  result in `test_after.log`.

Completion check:

- Migrated BIR-origin publications use only applicable named BIR evidence plus
  prepared authority; prepared-originated `JoinTransfer` publications use only
  complete, unique prepared authority plus applicable named producer/control
  identity; no route fallback or synthesized BIR publication relation remains;
  public payloads contain no route-numbered executable state; and the broader
  backend proof is green without weakened supported cases.

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
