Status: Active
Source Idea Path: ideas/open/220_backend_test_tree_split_bir_mir_and_prune_legacy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create BIR Test Tree And Move BIR-Owned Tests

# Current Packet

## Just Finished

Step 2 BIR C++ CMake ownership packet completed. Moved the remaining
BIR-owned executable and `add_test` definitions that source
`tests/backend/bir/*.cpp` from `tests/backend/CMakeLists.txt` into
`tests/backend/bir/CMakeLists.txt`. Target names, test names, labels, source
paths, and link behavior were left unchanged.

## Suggested Next

Next Step 2 recommendation: inspect the remaining root backend CMake route
tests for x86/riscv semantic-route assertions that observe BIR or prepared-BIR
behavior, then delegate a mechanical BIR ownership move for only those route
test definitions if the source/test ownership is unambiguous.

## Watchouts

- The BIR CLI dump/prepared-dump and BIR C++ executable/test definitions now
  live under BIR CMake ownership.
- Keep the next packet ownership-driven: route tests should move only when
  their assertion target is BIR/prepared-BIR, not just because they compile
  through the backend route.
- Do not move AArch64 tests, case fixtures, disabled MIR placeholders, or
  implementation files as part of Step 2.
- Disabled MIR dump/trace placeholders are still a pruning concern, not a move
  target.

## Proof

Delegated proof command:
`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "^(backend_prepare_|backend_lir_to_bir_notes|backend_prepared_printer|backend_x86_prepared_handoff_label_authority)" --output-on-failure' > test_after.log 2>&1`

Result: passed. CMake regenerated, the moved BIR C++ targets built under
`tests/backend/bir`, and the focused CTest subset passed 10/10 tests. Proof log
path is `test_after.log`.
