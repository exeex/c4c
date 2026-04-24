Status: Active
Source Idea Path: ideas/open/bir-agent-index-header-hierarchy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Split Memory Lowering Into A Subdirectory

# Current Packet

## Just Finished

Completed `plan.md` Step 4, `Split Memory Lowering Into A Subdirectory`.
Moved the memory-family LIR-to-BIR implementation files under
`src/backend/bir/lir_to_bir/memory/` without changing lowering semantics:
`memory.cpp` is now `memory/coordinator.cpp`, and the addressing, local-slot,
provenance, and value-materialization files now live beside it with shorter
subdomain filenames.

Updated the moved files' relative `lowering.hpp` include and the explicit
`backend_lir_to_bir_notes_test` source list in `tests/backend/CMakeLists.txt`.
No `memory/memory.hpp` was added because the moved files still share the
existing private `lir_to_bir/lowering.hpp` declarations.

## Suggested Next

Execute `plan.md` Step 5, `Final Structural Validation`, by inspecting the
final header/source layout, confirming no thin one-off headers were introduced,
and running the supervisor-selected broader backend validation.

## Watchouts

- This initiative remains structural; do not change lowering semantics or
  testcase expectations.
- Do not edit `ideas/open/bir-agent-index-header-hierarchy.md` for routine
  execution notes.
- Do not create one header per implementation file.
- The memory split deliberately did not add `memory/memory.hpp`; shared
  declarations still live in `src/backend/bir/lir_to_bir/lowering.hpp`.
- `src/backend/CMakeLists.txt` uses a recursive BIR source glob, while
  `tests/backend/CMakeLists.txt` has an explicit notes-test source list that
  must be kept in sync with future source moves.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
The build completed and the backend subset passed: 97 tests passed, 0 failed,
with 12 disabled tests not run. Proof log: `test_after.log`.
