Status: Active
Source Idea Path: ideas/open/01_bir-memory-header-vocabulary-extraction.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Create Memory Helpers Header

# Current Packet

## Just Finished

Completed `Step 3: Create Memory Helpers Header` by adding
`src/backend/bir/lir_to_bir/memory/memory_helpers.hpp` as the shared private
declaration surface for pure memory layout/projection helpers. `lowering.hpp`
now includes that helper index from the local/global memory helper section
while remaining the complete private `BirFunctionLowerer` index.

## Suggested Next

Execute `Step 4: Consolidate Shared Layout Projection Helpers` by moving or
consolidating repeated pure helper logic behind `memory_helpers.hpp` where
semantics match, without moving stateful policy out of the current
implementation files.

## Watchouts

- The two allowed memory headers across this initiative now exist:
  `memory_types.hpp` and `memory_helpers.hpp`; do not add per-implementation
  memory headers.
- Keep `lowering.hpp` as the complete private `BirFunctionLowerer` index.
- Preserve behavior and testcase expectations.
- `memory_helpers.hpp` currently indexes declarations only; Step 4 owns any
  semantic consolidation of repeated layout/projection helper implementations.

## Proof

Ran the delegated proof command
`cmake --build --preset default --target c4c_backend`. Passed. Proof log:
`test_after.log`.
