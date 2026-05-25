Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Separate Publication And Producer Lookup

# Current Packet

## Just Finished

Step 3 - Separate Publication And Producer Lookup moved the remaining
same-block/current-block join parallel-copy source lookup helpers out of
`dispatch.cpp` into `dispatch_producers.cpp`, with the cache/query API exposed
from `dispatch_producers.hpp`.

Behavior is preserved: `dispatch.cpp` still builds the join parallel-copy cache
at the block routing point and only asks producer-owned helpers whether the
current instruction is an incoming expression or source to skip.

## Suggested Next

Supervisor review/commit this Step 3 producer-lookup extraction slice, then
decide whether any remaining Step 3 dispatch-local lookup helpers belong in
`dispatch_producers` or whether Step 3 is ready for plan-owner review.

## Watchouts

`clang-format` is not installed in this environment, so the touched files were
manually kept in local style. No call-source files, call-specific retargeting,
fallback memory-lowering orchestration, or expectation/source tests changed.

## Proof

Passed. Proof log: `test_after.log`.

```bash
cmake --build --preset default && ctest --test-dir build -R 'backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records' --output-on-failure > test_after.log 2>&1
```
