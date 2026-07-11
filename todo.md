Status: Active
Source Idea Path: ideas/open/694_bir_route_index_retirement_umbrella.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Generate Focused Open Ideas

# Current Packet

## Just Finished

Step 4: Generate Focused Open Ideas completed. Generated ordered follow-up
ideas from the route-retirement handoff:

1. `ideas/open/695_bir_route_facade_named_compatibility_adapters.md`
2. `ideas/open/696_bir_producer_index_view_extraction.md`
3. `ideas/open/697_bir_memory_publication_view_extraction.md`
4. `ideas/open/698_bir_call_return_route_view_extraction.md`
5. `ideas/open/699_prealloc_named_bir_proof_consumer_migration.md`
6. `ideas/open/700_prepared_mir_stack_view_contract.md`
7. `ideas/open/701_route_fact_test_dump_contract_cleanup.md`
8. `ideas/open/702_residual_stack_authority_revisit_prerequisites.md`

Each generated idea names owned files, the first owning layer, the first
consumer or producer migration, the proof surface, numbered route APIs that
remain private compatibility, and concrete reviewer reject signals. The split
keeps BIR view extraction, prealloc consumer migration, prepared/MIR stack
authority, test/dump policy, and residual stack authority prerequisites in
separate owning layers.

## Suggested Next

Start Step 5 in `plan.md`: prepare umbrella closure evidence in `todo.md`,
including the consumed research docs, handoff docs written, generated
follow-up idea order, route-numbered APIs that remain private compatibility,
and the prerequisite before ideas 647 and 655 can be revisited.

## Watchouts

Route-numbered APIs remain private compatibility during migration. Ideas 647
and 655 remain parked unless positive prepared producer evidence appears above
route dumps. Do not merge BIR view extraction, prealloc consumer migration,
prepared/MIR authority, test policy, or residual stack authority repair into a
single implementation packet.

## Proof

Lifecycle-only proof ran:
`test -f ideas/open/695_bir_route_facade_named_compatibility_adapters.md -a -f ideas/open/696_bir_producer_index_view_extraction.md -a -f ideas/open/697_bir_memory_publication_view_extraction.md -a -f ideas/open/698_bir_call_return_route_view_extraction.md -a -f ideas/open/699_prealloc_named_bir_proof_consumer_migration.md -a -f ideas/open/700_prepared_mir_stack_view_contract.md -a -f ideas/open/701_route_fact_test_dump_contract_cleanup.md -a -f ideas/open/702_residual_stack_authority_revisit_prerequisites.md && rg -n "Owned Files|First Owning Layer|First (Consumer|Producer) Migration|Proof Surface|Numbered Route APIs Kept Private Compatibility|Reviewer Reject Signals" ideas/open/69{5,6,7,8}_*.md ideas/open/699_*.md ideas/open/70{0,1,2}_*.md`

No build was required for this lifecycle-only packet. The delegated proof did
not write `test_after.log`; no root-level log file was created.
