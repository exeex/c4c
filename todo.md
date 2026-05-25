Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Separate Publication And Producer Lookup

# Current Packet

## Just Finished

Step 3 - Separate Publication And Producer Lookup moved
`retarget_memory_result_to_prepared_home` out of `dispatch.cpp` into
`dispatch_publication.cpp`, with the owner surface declared in
`dispatch_publication.hpp`.

The direct support helpers now live privately in `dispatch_publication.cpp`:
`before_return_move_targets_fpr_abi`,
`memory_load_result_feeds_before_return_fpr_abi`,
`find_storage_plan_value`, and
`symbol_fp_load_has_explicit_storage_placement`.

Existing dispatch call sites still retarget memory and f128 transport results
through the publication owner. Behavior is preserved for symbol loads, frame
slot loads feeding before-return FPR ABI publication, explicit FPR storage
placement exclusion, and prepared-home register retargeting.

## Suggested Next

Supervisor review/commit this Step 3 memory-result publication retargeting
slice, then select the next publication/producer lookup extraction target.

## Watchouts

No store-value retargeting helpers, address-materialization orchestration,
producer-cache helpers, branch/compare missing-publication helpers,
call-source files, or call-boundary materialization helpers changed.
Remaining Step 3 candidates include store/memory publication retargeting,
before-return publication recording, branch/compare missing-publication helpers,
and the current-block join producer/cache cluster.

## Proof

Passed. Proof log: `test_after.log`.

```bash
cmake --build --preset default && ctest --test-dir build -R 'backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_return_lowering' --output-on-failure > test_after.log 2>&1
```
