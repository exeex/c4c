Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Separate Publication And Producer Lookup

# Current Packet

## Just Finished

Step 3 - Separate Publication And Producer Lookup moved the fixed-formal
store-local publication helpers out of `dispatch.cpp` into
`dispatch_publication.cpp`, with `lower_fixed_formal_store_local_publication`
exposed from `dispatch_publication.hpp`.

Behavior is preserved: `dispatch.cpp` still calls the publication-owned helper
at the same store-local address-materialization fallback routing points, and the
same emitted-scalar lookup, prepared frame-slot address lookup, inline-asm
payload construction, and diagnostic suppression behavior is retained.

## Suggested Next

Supervisor review/commit this Step 3 publication extraction slice, then check
whether any remaining Step 3 dispatch-local helper ownership still belongs with
publication/producer lookup or whether Step 3 is ready for plan-owner review.

## Watchouts

`clang-format` is not installed in this environment, so the touched files were
manually kept in local style. No call-source files, call-specific retargeting,
fallback memory-lowering orchestration, or expectation/source tests changed.

## Proof

Passed. Proof log: `test_after.log`.

```bash
cmake --build --preset default && ctest --test-dir build -R 'backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records' --output-on-failure > test_after.log 2>&1
```
