# BIR Route Index Retirement Umbrella

Status: Closed
Type: Umbrella triage and follow-up idea generator
After: `ideas/closed/693_bir_route_index_retirement_research.md`
Parent: `ideas/closed/693_bir_route_index_retirement_research.md`
Handoff Directory: `docs/bir_route_index_retirement/`
Consumes:
- `docs/bir_route_index_retirement_research/index.md`
- `docs/bir_route_index_retirement_research/01_current_route_inventory.md`
- `docs/bir_route_index_retirement_research/02_required_bir_to_prealloc_inputs.md`
- `docs/bir_route_index_retirement_research/03_named_view_replacement_shape.md`
- `docs/bir_route_index_retirement_research/04_publication_and_authority_boundaries.md`
- `docs/bir_route_index_retirement_research/05_retirement_sequence.md`
- `docs/bir_route_index_retirement_research/06_test_and_dump_policy_after_route_retirement.md`
- `docs/bir_route_index_retirement_research/07_followup_idea_recommendations.md`
- `docs/bir_route_index_retirement_research/08_stack_view_and_destination_authority_handoff.md`
Related:
- `ideas/closed/693_bir_route_index_retirement_research.md`
- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
- `ideas/open/655_stack_destination_fan_in_authority_decomposition.md`
- `src/backend/bir/`
- `src/backend/prealloc/`
- `tests/backend/bir/CMakeLists.txt`
- `tests/backend/CMakeLists.txt`

## Goal

Use the route-retirement research docs to classify the `src/backend/bir`
route-numbered middle layer and generate ordered follow-up ideas that clean up
the BIR-to-prealloc boundary without relying on default intermediate fact dump
tests.

## Why This Exists

The compiler now has connected proof across frontend, BIR, prepared MIR view,
MIR, object, and runtime paths. Default CTest no longer needs to pin every
intermediate prepared/BIR fact dump. That creates room to clean up the old
route-numbered BIR APIs instead of preserving them as observable contracts.

Direct implementation is still premature until the research output is digested
into a dependency-ordered queue. The umbrella should prevent route drift from
"rename route files" into broad BIR/prealloc rewrite or residual stack
destination testcase repair.

## Current Evidence

- Required input evidence from
  `docs/bir_route_index_retirement_research/`.
- Existing LIR-to-BIR adapter cleanup evidence from
  `docs/lir_bir_adapter_boundary/`.
- Prepared MIR view boundary evidence from
  `docs/prepared_mir_view_contract_research/`.
- Current default test policy that gates prepared fact tests out of default
  CTest while keeping MIR/object/runtime proof.
- Parked residual stack destination ideas 647 and 655, which must not be
  reactivated until the BIR route/publication authority boundary is clearer.

## In Scope

- Create `docs/bir_route_index_retirement/` as the durable handoff directory.
- Summarize the research results into an implementation queue.
- Generate ordered follow-up ideas under `ideas/open/`.
- Classify each follow-up by first owning layer:
  BIR named view extraction, route facade contraction, BIR publication boundary,
  prealloc consumer migration, prepared/MIR stack view contract, test/dump
  contract cleanup, or residual stack authority prerequisite.
- Record which route-numbered APIs may remain private compatibility during
  migration.

## Out Of Scope

- Implementation changes inside the umbrella.
- Rewriting BIR-to-prealloc directly.
- Reactivating ideas 647 or 655 without a named prerequisite from the route
  retirement docs.
- Reintroducing default intermediate prepared/BIR fact dump checks as the main
  proof mechanism.
- Test expectation rewrites, unsupported-marker edits, allowlist edits,
  timeout changes, runtime behavior changes, or baseline-policy changes.

## Priority Model

Order follow-up ideas by dependency and blast-radius reduction:

1. Public route facade contraction and named view aliases with no behavior
   change.
2. Consumer migration from numbered route APIs to named BIR views.
3. Route publication/authority boundary cleanup.
4. Stack/frame/value-home/destination-authority handoff cleanup.
5. Test and dump contract retirement or gating cleanup.
6. Residual stack destination authority revisit only after the BIR publication
   boundary exposes a positive producer seam.

Prefer small behavior-preserving extractions with fresh build and focused
backend proof before semantic repairs.

## Required Follow-Up Ideas

Generate at least these follow-up families unless the research proves a better
split:

- `BIR producer/index view extraction`: owning layer canonical BIR route
  producer lookup.
- `BIR memory/publication view extraction`: owning layer BIR route publication
  and memory observation.
- `BIR call/return route view extraction`: owning layer BIR call boundary and
  return-chain route facts.
- `Prealloc route consumer migration`: owning layer BIR-to-prealloc consumer
  interface.
- `Prepared MIR stack view contract`: owning layer prealloc/prepared stack fact
  production and MIR stack consumption.
- `Route fact test and dump contract cleanup`: owning layer backend test policy
  and debug dump compatibility.

Each generated follow-up must name owned files, first consumer migration, proof
surface, and which numbered route APIs it must not expose as new public
architecture.

## Acceptance Criteria

- `docs/bir_route_index_retirement/` contains a research digest, ownership
  classification, and ordered follow-up plan.
- The documents agree on the same research input set from idea 693.
- Follow-up ideas are generated under `ideas/open/` and ordered by dependency.
- Each follow-up idea names its first owning layer and avoids mixing BIR view
  extraction, prealloc consumer migration, test policy, and stack authority
  repair.
- The handoff explicitly records whether stack/frame/value-home and
  destination-authority facts belong to BIR semantic views, prepared/prealloc
  producers, or MIR consumers.
- The umbrella does not change implementation, tests, expectations,
  unsupported markers, allowlists, runtime behavior, default harness contracts,
  or baseline policy.

## Closure Note Requirements

The closure note must state which research docs were consumed, which handoff
documents were written, which follow-up ideas were generated, how they were
ordered, what route-numbered APIs remain as private compatibility, and what
must happen before ideas 647 and 655 can be revisited.

## Closure Note

Closed after the umbrella runbook produced the durable handoff package under
`docs/bir_route_index_retirement/` and generated the ordered follow-up queue.

Consumed research docs:
`docs/bir_route_index_retirement_research/index.md`,
`01_current_route_inventory.md`,
`02_required_bir_to_prealloc_inputs.md`,
`03_named_view_replacement_shape.md`,
`04_publication_and_authority_boundaries.md`,
`05_retirement_sequence.md`,
`06_test_and_dump_policy_after_route_retirement.md`,
`07_followup_idea_recommendations.md`, and
`08_stack_view_and_destination_authority_handoff.md`.

Handoff docs written:
`docs/bir_route_index_retirement/research_digest.md`,
`docs/bir_route_index_retirement/ownership_dependencies.md`, and
`docs/bir_route_index_retirement/ordered_followup_plan.md`.

Generated follow-up ideas, in dependency order:

1. `ideas/open/695_bir_route_facade_named_compatibility_adapters.md`
2. `ideas/open/696_bir_producer_index_view_extraction.md`
3. `ideas/open/697_bir_memory_publication_view_extraction.md`
4. `ideas/open/698_bir_call_return_route_view_extraction.md`
5. `ideas/open/699_prealloc_named_bir_proof_consumer_migration.md`
6. `ideas/open/700_prepared_mir_stack_view_contract.md`
7. `ideas/open/701_route_fact_test_dump_contract_cleanup.md`
8. `ideas/open/702_residual_stack_authority_revisit_prerequisites.md`

The ordering contracts the narrow Route 4/Route 7 facade first, then extracts
producer, publication, call, and return BIR views, then migrates prealloc proof
consumers, then defines prepared/MIR stack authority, then cleans test/dump
contracts, and leaves residual stack authority prerequisites last.

Route-numbered APIs remain private compatibility during migration:
`RouteIndexReferenceFacade`, `RouteIndexRoute`,
`RouteIndexRecordReference`, `Route4IndexReferenceValidation`,
`Route7IndexReferenceValidation`, `Route4PublicationAvailabilityIndex`,
`Route5EdgeJoinSourceIndex`, Route 5 CFG-edge and current-block join-source
rows, `route5_join_source`, `route5_join_source_status`,
`route5_join_source_agrees`, `Route7ComparisonConditionIndex`,
`Route1ProducerIndex`, `Route2SelectChainValueIndex`,
`Route3MemoryAccessIndex`, `Route6CallUseSourceIndex`, and
`Route8ReturnChainIndex`. They may remain as private builders, proof adapters,
diagnostic bridges, rollback inputs, or compatibility fields, but they must
not become prepared publication, source freshness, move-bundle, value-home,
stack destination, frame-layout, branch stack-load, or MIR authority.

Ideas 647 and 655 remain parked until prepared/MIR stack view work exposes
positive prepared producer evidence above route dumps, including named
destination value/home/storage authority, selected move or freshness authority,
stack-source or branch stack-load authority when relevant, fail-closed MIR
statuses, and prepared/MIR/object/runtime proof if executable behavior changes.
Route 4, Route 5, Route 7, facade status, dump rows, expectations, and
allowlists remain compatibility evidence only.

## Reviewer Reject Signals

- Reject direct implementation inside the umbrella idea.
- Reject output that only renames route numbers without ownership
  classification and follow-up ordering.
- Reject follow-up ideas that mix BIR named view extraction with prealloc
  consumer migration, test-policy changes, or stack destination authority
  repair.
- Reject follow-up ideas that make MIR rediscover stack destination authority
  from raw route records instead of consuming explicit prepared/prealloc facts.
- Reject reintroducing intermediate dump comparisons as the main correctness
  proof after the connected MIR/object/runtime proof path exists.
- Reject testcase-shaped shortcuts, expectation rewrites, unsupported
  downgrades, allowlist filtering, timeout/accounting changes, or weaker
  runtime checks as progress.
