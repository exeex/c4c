Status: Active
Source Idea Path: ideas/open/266_aarch64_atomics_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Delete The Stale Markdown Shard

# Current Packet

## Just Finished

Completed Step 2 by creating `src/backend/mir/aarch64/codegen/atomics.cpp`
and `atomics.hpp` as the compiled AArch64 atomics owner boundary.

Moved current-route atomic ownership out of `memory.cpp`/`memory.hpp`:

- atomic record error naming and atomic instruction kind naming
- prepared atomic carrier validation and record construction
- selected atomic width, ordering, RMW opcode, result-mode, and storage
  authority checks
- fail-closed diagnostics for incomplete carriers, unsupported forms, missing
  authority, and scratch/register conversion failures
- `make_atomic_memory_instruction(...)` selection, defs/uses, side effects,
  and atomic machine opcode routing
- `lower_atomic_memory_operations_for_block(...)` prepared-carrier scan and
  high-level lowering entry point

`dispatch.cpp` now includes `atomics.hpp` and still calls the same high-level
`lower_atomic_memory_operations_for_block(...)` entry point. `memory.cpp` keeps
ordinary memory lowering only. `machine_printer.*`, tests, markdown shards,
`plan.md`, and `ideas/open/*` were not touched.

## Suggested Next

Proceed to Step 3 by deleting `src/backend/mir/aarch64/codegen/atomics.md`
and updating only required source/docs index references that still expect the
stale markdown shard.

## Watchouts

- The new owner intentionally preserves the existing prepared-route subset; it
  does not port the legacy fixed-register markdown behavior.
- Final assembly spelling still lives in `machine_printer.cpp`; printer
  behavior was not changed.
- Instruction payload declarations remain in `instruction.hpp`.

## Proof

Ran:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: PASS, backend subset `139/139` passed.

Proof log: `test_after.log`
