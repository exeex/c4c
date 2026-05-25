Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Separate Publication And Producer Lookup

# Current Packet

## Just Finished

Step 3 - Separate Publication And Producer Lookup moved
`record_address_materialization_result` and `record_memory_result` out of
`dispatch.cpp` into `dispatch_publication.cpp`, with declarations exposed in
`dispatch_publication.hpp`.

Dispatch still calls the helpers at the existing address-materialization and
memory-result recording sites. Behavior is preserved: address materialization
records publish their result register into scalar state, and memory load
records publish their result register unless they are stack-result records.

## Suggested Next

Supervisor review/commit this Step 3 scalar-state publication recording slice,
then select the next publication/producer lookup extraction target.

## Watchouts

No address-materialization orchestration, producer-cache extraction,
call-source files, or call-boundary retargeting/materialization helpers changed.
Remaining Step 3 candidates include before-return publication recording,
memory/store retargeting, branch/compare missing-publication helpers, and the
current-block join producer/cache cluster.

## Proof

Passed. Proof log: `test_after.log`.

```bash
cmake --build --preset default && ctest --test-dir build -R 'backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records' --output-on-failure > test_after.log 2>&1
```
