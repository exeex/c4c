Status: Active
Source Idea Path: ideas/open/11_aarch64_calls_file_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Retire Remaining Stale Calls Build Entries

# Current Packet

## Just Finished

Step 4 - Retire Remaining Stale Calls Build Entries completed as a
metadata-only cleanup verification. No stale `calls*` source/build entries were
found after the adapter/header consolidation, so no implementation, include, or
CMake edit was needed.

Checks used:

- `git ls-files 'src/backend/mir/aarch64/codegen/calls*'` showed the tracked
  calls files are exactly `calls.cpp`, `calls.hpp`, `calls_byval_aggregates.cpp`,
  `calls_common.cpp`, `calls_dispatch_bridge.cpp`,
  `calls_dispatch_bridge.hpp`, and `calls_moves.cpp`.
- `rg` over source, backend CMake, and focused backend tests found only active
  calls build entries/includes/usages; retired names such as
  `calls_argument_sources.cpp` and `calls_printing.cpp` do not appear in active
  source/build/test references.
- `c4c-clang-tool function-signatures` inventoried `calls.hpp` and confirmed
  the remaining declarations are active cross-TU APIs.
- `c4c-clang-tool-ccdb list-symbols` inspected each remaining calls translation
  unit and confirmed they still own active behavior:
  `calls.cpp` owns prepared call lookup/lowering, `calls_common.cpp` owns common
  call utilities and target operand adapters, `calls_byval_aggregates.cpp` owns
  byval aggregate lane helpers, `calls_moves.cpp` owns call-boundary/value move
  lowering, and `calls_dispatch_bridge.cpp` owns the dispatch bridge call
  materialization/publication behavior.

Changed files:

- `todo.md`

## Suggested Next

Supervisor should review/commit this completed Step 4 metadata-only
verification. Suggested Step 5 validation is an acceptance-oriented run that
keeps the same focused backend subset green after the stale-entry audit, then
escalates to the supervisor-chosen broader guard if this closes the calls
consolidation milestone.

## Watchouts

- This packet made no implementation, build, include, semantic lowering,
  dispatch bridge, ABI classification, or test expectation changes.
- `src/backend/CMakeLists.txt` still intentionally lists active calls
  translation units: `calls.cpp`, `calls_byval_aggregates.cpp`,
  `calls_common.cpp`, `calls_dispatch_bridge.cpp`, and `calls_moves.cpp`.
- Leave unrelated transient `review/` artifacts untouched.
- Do not fold `calls_dispatch_bridge.*`, `dispatch.cpp`, or broad
  `calls_moves.cpp` behavior into this consolidation route.

## Proof

Ran the supervisor-selected proof and preserved output in `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_prepare_frame_stack_call_contract|backend_aarch64_target_instruction_records)$'`

Result: build succeeded; 4/4 focused tests passed.
