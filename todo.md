Status: Active
Source Idea Path: ideas/open/220_backend_test_tree_split_bir_mir_and_prune_legacy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create BIR Test Tree And Move BIR-Owned Tests

# Current Packet

## Just Finished

Step 2 BIR CLI CMake ownership packet completed. Created
`tests/backend/bir/CMakeLists.txt`, moved the BIR-owned
`backend_cli_dump_bir*` and `backend_cli_dump_prepared_bir*` test definitions
there, and replaced the old root definitions with `add_subdirectory(bir)`.
Test names, labels, command arguments, and behavior were left unchanged.

## Suggested Next

Next Step 2 mechanical move packet: move the remaining BIR-owned C++ executable
and `add_test` definitions that already source `tests/backend/bir/*.cpp` from
`tests/backend/CMakeLists.txt` into `tests/backend/bir/CMakeLists.txt`.
Preserve target names, test names, labels, source paths, and link behavior.

## Watchouts

- The BIR CLI dump/prepared-dump definitions now live under BIR CMake ownership,
  but the BIR C++ executable/test definitions are still rooted in
  `tests/backend/CMakeLists.txt`.
- Keep the next packet mechanical: do not move backend route tests, AArch64
  tests, case fixtures, disabled MIR placeholders, or implementation files.
- Disabled MIR dump/trace placeholders are still a pruning concern, not a move
  target.

## Proof

Delegated proof command:
`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "^backend_cli_dump_(bir|prepared_bir)" --output-on-failure' > test_after.log 2>&1`

Result: passed. CMake regenerated, the build completed with no work needed, and
the focused CTest subset passed 19/19 tests. Proof log path is
`test_after.log`.
