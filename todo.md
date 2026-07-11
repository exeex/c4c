Status: Active
Source Idea Path: ideas/open/694_bir_route_index_retirement_umbrella.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prepare Umbrella Closure

# Current Packet

## Just Finished

Step 5: Prepare Umbrella Closure completed. This todo-only closure evidence is
package-ready for plan-owner close review and records no implementation, test,
expectation, allowlist, runtime, default harness, baseline, or behavior change.

The consumed research docs from `docs/bir_route_index_retirement_research/`:
`index.md`, `01_current_route_inventory.md`,
`02_required_bir_to_prealloc_inputs.md`,
`03_named_view_replacement_shape.md`,
`04_publication_and_authority_boundaries.md`,
`05_retirement_sequence.md`,
`06_test_and_dump_policy_after_route_retirement.md`,
`07_followup_idea_recommendations.md`, and
`08_stack_view_and_destination_authority_handoff.md`.

The handoff documents written under `docs/bir_route_index_retirement/`:
`research_digest.md`, `ownership_dependencies.md`, and
`ordered_followup_plan.md`. These documents record the consumed input set,
ownership/dependency split, ordered follow-up queue, route-numbered private
compatibility boundary, and residual stack-authority prerequisites.

Generated follow-up ideas and ordering:

1. `ideas/open/695_bir_route_facade_named_compatibility_adapters.md` - first
   contracts the narrow Route 4/Route 7 facade through named compatibility
   adapters without changing behavior.
2. `ideas/open/696_bir_producer_index_view_extraction.md` - extracts producer
   and control-value named BIR views after the facade adapter foothold exists.
3. `ideas/open/697_bir_memory_publication_view_extraction.md` - splits memory
   and publication BIR facts from prepared publication authority.
4. `ideas/open/698_bir_call_return_route_view_extraction.md` - isolates call
   and return-chain route families behind named BIR views.
5. `ideas/open/699_prealloc_named_bir_proof_consumer_migration.md` - moves the
   first prealloc proof consumers from numbered routes to named BIR proof
   views while preserving prepared facts.
6. `ideas/open/700_prepared_mir_stack_view_contract.md` - defines prepared/MIR
   stack, frame, value-home, move, freshness, and destination-authority
   contracts before MIR consumes stack destinations.
7. `ideas/open/701_route_fact_test_dump_contract_cleanup.md` - rewrites route
   dump/test vocabulary only after named proof surfaces exist.
8. `ideas/open/702_residual_stack_authority_revisit_prerequisites.md` - leaves
   residual stack authority last and prerequisite-driven.

Route-numbered APIs that remain private compatibility during migration:
`RouteIndexReferenceFacade`, `RouteIndexRoute`,
`RouteIndexRecordReference`, `Route4IndexReferenceValidation`,
`Route7IndexReferenceValidation`, `Route4PublicationAvailabilityIndex`,
`Route5EdgeJoinSourceIndex`, Route 5 CFG-edge/current-block join-source rows,
`route5_join_source`, `route5_join_source_status`,
`route5_join_source_agrees`, `Route7ComparisonConditionIndex`,
`Route1ProducerIndex`, `Route2SelectChainValueIndex`,
`Route3MemoryAccessIndex`, `Route6CallUseSourceIndex`, and
`Route8ReturnChainIndex`. They may remain as private builders, proof adapters,
diagnostic bridges, rollback inputs, or compatibility fields; they must not
become prepared publication, source freshness, move-bundle, value-home, stack
destination, frame-layout, branch stack-load, or MIR authority.

Before ideas 647 and 655 can be revisited, a prepared/MIR stack view contract
must expose positive prepared producer evidence above route dumps. Required
evidence includes named destination value/home/storage authority, selected move
bundle or move resolution, selected freshness, frame or aggregate stack-source
authority, branch stack-load authority when relevant, MIR fail-closed statuses,
and prepared contract, prepared MIR, object, object-runtime, or runtime proof
when executable behavior changes. Route 4, Route 5, Route 7, facade status,
dump rows, expectations, and allowlists remain compatibility evidence only and
are insufficient to reactivate 647 or 655.

## Suggested Next

Hand off to the supervisor for plan-owner close review. The umbrella closure
package is ready to review against `plan.md` Step 5 and
`ideas/open/694_bir_route_index_retirement_umbrella.md`; no executor-side
implementation packet is recommended from this active plan.

## Watchouts

The handoff document actually present for ownership classification is
`docs/bir_route_index_retirement/ownership_dependencies.md`. Do not merge BIR
view extraction, prealloc consumer migration, prepared/MIR authority, test
policy, or residual stack authority repair into a single implementation
packet. Route-numbered APIs remain private compatibility only during
migration.

## Proof

Lifecycle-only proof ran:
`rg -n "consumed research docs|handoff documents|follow-up ideas|private compatibility|647|655|closure evidence|no implementation" todo.md`

No build was required for this lifecycle-only packet. The delegated proof did
not require `test_after.log`; no root-level log file was created.
