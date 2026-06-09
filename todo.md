Status: Active
Source Idea Path: ideas/open/131_aarch64_dispatch_edge_copy_helper_surface_privatization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Clean Header And Build Metadata If Needed

# Current Packet

## Just Finished

Completed plan Step 4: Clean Header And Build Metadata If Needed.

Removed stale `dispatch_edge_copies.hpp` includes from implementation files
that do not use the two retained public hooks:

- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`

Retained direct users still include the header: `dispatch.cpp` and the focused
AArch64 instruction-dispatch tests. No build metadata change was needed.

## Suggested Next

Proceed to plan-owner review or closure for idea 131, if the supervisor agrees
that the Step 2 surface contraction, Step 3 no-move decision, and Step 4 include
cleanup complete the active runbook.

## Watchouts

- The four helper declarations remain absent from `dispatch_edge_copies.hpp`.
- `dispatch.cpp` and `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
  still directly use the retained public hooks and should keep the include.
- `src/backend/CMakeLists.txt` was unchanged because no translation unit was
  removed or added.

## Proof

Ran the exact delegated proof:

`bash -o pipefail -c "cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_'" > test_after.log 2>&1`

Result: passed. `cmake --build --preset default` completed, and the
`^backend_aarch64_` CTest subset reported 28/28 tests passed.

Supervisor regression guard: maintenance non-decreasing mode passed with
before=28/28 and after=28/28, no new failures.

Proof log path: `test_after.log`.
