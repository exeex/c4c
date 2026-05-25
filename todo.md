Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Separate Publication And Producer Lookup

# Current Packet

## Just Finished

Step 3 - Separate Publication And Producer Lookup moved
`retarget_pointer_store_value_to_materialized_address` and
`retarget_store_address_to_materialized_pointer` out of `dispatch.cpp` into
`dispatch_publication.cpp`, with declarations exposed in
`dispatch_publication.hpp`.

Dispatch still calls the helpers from `lower_store_local_with_address_materialization`
after address materialization succeeds. Behavior is preserved: pointer store
values can be retargeted to the materialized address register, and pointer-value
store addresses can be rewritten to a concrete register-backed memory address.

## Suggested Next

Supervisor review/commit this Step 3 pointer-store retargeting slice, then
select the next publication/producer lookup extraction target.

## Watchouts

No address-materialization orchestration, fallback memory-lowering
orchestration, store-value emitted-scalar retargeting helpers, producer-cache
helpers, branch/compare missing-publication helpers, call-source files, or
call-boundary materialization helpers changed. Remaining Step 3 candidates
include store-value emitted-scalar retargeting, before-return publication
recording, branch/compare missing-publication helpers, and the current-block
join producer/cache cluster.

## Proof

Passed. Proof log: `test_after.log`.

```bash
cmake --build --preset default && ctest --test-dir build -R 'backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records' --output-on-failure > test_after.log 2>&1
```
