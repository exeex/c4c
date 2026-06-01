Status: Active
Source Idea Path: ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Consume Prepared Stack-Source Authorities

# Current Packet

## Just Finished

Completed the Step 4 aggregate stack-source inspection packet. Checked the
existing `memory.cpp` prepared edge-publication consumers around
`find_unique_load_result_stack_source_publication(...)`,
`make_prepared_load_memory_instruction_record(...)` for local/global loads,
`plan_store_local_source_publication(...)`,
`lower_store_local_value_publication(...)`,
`lower_store_global_value_publication_from_plan(...)`, and
`lower_store_global_value_publication(...)`. No in-scope consumer was wired:
`PreparedAggregateStackSourceAuthority` is populated on
`PreparedEdgePublication`, but `memory.cpp` does not reference that type, and
the store-source publication plan/source-producer path does not expose the
originating `PreparedEdgePublication` or its aggregate authority.

## Suggested Next

Next packet: supervisor/plan-owner route decision. Either add a narrowly scoped
prepared lookup/fact boundary that exposes the originating edge publication or
aggregate stack-source authority to the store-source publication lowering path,
or defer aggregate stack-source transport to the later aggregate-copy planning
step.

## Watchouts

Exact blocker: the only existing `memory.cpp` prepared edge-publication lookup
boundary is the local/global load-result typed helper, which calls
`prepare::prepare_same_width_i32_stack_source_publication(...)` from a unique
load-result `PreparedEdgePublication`. Aggregate authority lives on
`PreparedEdgePublication::aggregate_stack_source_authority`, but the
store-local/store-global publication helpers consume only
`PreparedStoreSourcePublicationPlan` and source-producer facts. Consuming
`PreparedAggregateStackSourceAuthority` here would require adding a new
edge-publication lookup/fact boundary or aggregate-copy authority transport,
which is outside this Step 4 packet.

## Proof

Blocked before code changes. Per packet instructions, did not run the delegated
build/CTest proof and did not create a new `test_after.log` for this blocked
inspection. Ran:

```sh
git diff --check -- todo.md
```
