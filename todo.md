Status: Active
Source Idea Path: ideas/open/207_route3_memory_source_semantic_reader.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Agreement-Gated Route 3 Read

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by adding an agreement-gated Route 3 read to `prepared_current_global_load_access(...)` in `src/backend/mir/aarch64/codegen/globals.cpp`.

The helper still computes the prepared answer first through `prepare::find_prepared_global_load_access(...)`. When prepared access exists and the current BIR block is available, it reads `mir::find_bir_same_block_global_load_access_identity(...)` at `instruction_index + 1` so the current load is visible to the same-block Route 3 query, accepts the Route 3 load pointer only when it agrees with the prepared load instruction, result value name/type, block, instruction index, global-symbol base/id, address space, volatility, offset, size, and alignment, and otherwise returns the original prepared result.

Added focused `backend_aarch64_instruction_dispatch` coverage for the GOT-required global-load path proving the matching Route 3 identity still emits GOT materialization and prepared fallback is preserved when Route 3 evidence is absent, non-global, symbol-mismatched, or memory-flag-mismatched. Existing `backend_aarch64_prepared_memory_operand_records` coverage remains the focused Route 3 wrong-type/non-global query surface.

## Suggested Next

Review Step 2 for route quality and decide the next packet from the active plan. A natural next implementation packet is the next selected semantic reader, if the supervisor wants to continue Route 3 migration after accepting this slice.

## Watchouts

- The selected helper now uses `instruction_index + 1` for the Route 3 same-block `before_instruction_index`; this is intentional because Route 3 same-block memory lookup excludes records at or after the before index.
- Prepared memory/addressing remains authoritative for address policy, GOT-vs-direct selection, final operands, scratch/target registers, relocation spelling, and emission.
- `clang-format` was not available in this environment, so formatting was kept manually consistent with nearby code.

## Proof

Ran the supervisor-selected proof exactly:
`cmake --build --preset default --target backend_aarch64_prepared_memory_operand_records_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch)$' > test_after.log`

Result: passed, 2/2 tests passed. Proof log: `test_after.log`.
