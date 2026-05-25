Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Separate Publication And Producer Lookup

# Current Packet

## Just Finished

Step 3 - Separate Publication And Producer Lookup moved
`retarget_pointer_store_value_to_emitted_scalar` and
`retarget_store_local_value_to_emitted_scalar` out of `dispatch.cpp` into
`dispatch_publication.cpp`, with declarations exposed in
`dispatch_publication.hpp`.

Behavior is preserved: pointer store values can be retargeted to an already
emitted scalar pointer register, and store-local values using pointer-value
addresses can be retargeted to the prepared or emitted store value register
when that avoids a stale physical register reference.

## Suggested Next

Supervisor review/commit this Step 3 non-call store-value retargeting slice,
then select the next publication/producer lookup extraction target.

## Watchouts

The compile proof required a non-behavioral include adjustment:
`dispatch_publication.cpp` now includes `memory_store_sources.hpp` for
`store_local_uses_pointer_value_address` and
`prepared_or_emitted_store_value_register`.

`retarget_fpr_call_result_store_value_to_emitted_scalar` remains in
`dispatch.cpp` because it is call-result specific. No fallback memory-lowering
orchestration, `lower_store_local_value_publication`, call-source files,
before/after-call source retargeting, producer-cache helpers, or branch/compare
missing-publication helpers changed.

## Proof

Passed. Proof log: `test_after.log`.

```bash
cmake --build --preset default && ctest --test-dir build -R 'backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records' --output-on-failure > test_after.log 2>&1
```
