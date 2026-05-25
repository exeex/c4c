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

Step 3 appears ready to hand off to the next runbook step. Remaining
dispatch-local helper ownership is edge-copy handling, call-boundary glue, or
general traversal rather than publication/producer lookup.

## Watchouts

`clang-format` is not installed in this environment, so the touched files were
manually kept in local style. No call-source files, call-specific retargeting,
fallback memory-lowering orchestration, or expectation/source tests changed.

## Proof

Passed. Narrow proof log was rolled into `test_before.log`.

```bash
cmake --build --preset default && ctest --test-dir build -R 'backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records' --output-on-failure > test_after.log 2>&1
```

Broader backend checkpoint passed after the Step 3 extraction series:
162/162 backend tests passed before and after, with no new failures.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_before.log 2>&1
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```
