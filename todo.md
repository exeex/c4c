Status: Active
Source Idea Path: ideas/open/01_bir-memory-header-vocabulary-extraction.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Create Memory Types Header

# Current Packet

## Just Finished

Completed `Step 1: Create Memory Types Header` by adding
`src/backend/bir/lir_to_bir/memory/memory_types.hpp` and moving the first
memory-specific structs/hash/data vocabulary out of `BirFunctionLowerer`.
`lowering.hpp` now includes the new header, keeps compatibility aliases, and
still directly owns the memory lowering state fields.

## Suggested Next

Execute `Step 2: Move Memory Map Aliases` by moving the memory map and set
aliases from `BirFunctionLowerer` into `memory_types.hpp` while keeping the
state fields owned directly by `BirFunctionLowerer`.

## Watchouts

- Add only the two allowed memory headers across this initiative:
  `memory_types.hpp` and `memory_helpers.hpp`.
- Keep `lowering.hpp` as the complete private `BirFunctionLowerer` index.
- Preserve behavior and testcase expectations.
- The Step 1 aliases in `BirFunctionLowerer` intentionally reduce churn for
  existing memory implementation files; Step 2 can move map aliases without
  changing the state-owner fields.

## Proof

Ran `cmake --build --preset default --target c4c_codegen` after cleaning the
target state. Passed. Also ran the narrow supplemental compile target
`cmake --build --preset default --target c4c_backend` because the delegated
target does not compile the touched BIR backend files. Passed. Proof log:
`test_after.log`.
