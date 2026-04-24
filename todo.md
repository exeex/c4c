Status: Active
Source Idea Path: ideas/open/01_bir-memory-header-vocabulary-extraction.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Move Memory Map Aliases

# Current Packet

## Just Finished

Completed `Step 2: Move Memory Map Aliases` by moving the source-idea memory
map and set aliases into `src/backend/bir/lir_to_bir/memory/memory_types.hpp`.
`lowering.hpp` now forwards the existing `BirFunctionLowerer` compatibility
aliases to the memory vocabulary header, and the lowerer still directly owns
the memory lowering state fields.

## Suggested Next

Execute `Step 3: Create Memory Helpers Header` by adding
`src/backend/bir/lir_to_bir/memory/memory_helpers.hpp` for shared pure memory
layout/projection helper declarations while keeping implementation control flow
in the existing memory `.cpp` files.

## Watchouts

- Add only the two allowed memory headers across this initiative:
  `memory_types.hpp` and `memory_helpers.hpp`; `memory_helpers.hpp` is still
  pending after Step 2.
- Keep `lowering.hpp` as the complete private `BirFunctionLowerer` index.
- Preserve behavior and testcase expectations.
- `memory_types.hpp` forward-declares `lir_to_bir_detail::GlobalAddress` so the
  global-address map aliases can live with the memory vocabulary without moving
  the non-memory `GlobalAddress` definition.

## Proof

Ran the delegated proof command
`cmake --build --preset default --target c4c_backend`. Passed. Proof log:
`test_after.log`.
