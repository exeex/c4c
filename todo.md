Status: Active
Source Idea Path: ideas/open/220_backend_test_tree_split_bir_mir_and_prune_legacy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create BIR Test Tree And Move BIR-Owned Tests

# Current Packet

## Just Finished

Step 2 first mechanical move packet completed. Created `tests/backend/bir/` and
moved the obvious BIR-owned C++ unit tests there:
`backend_prepare_*_test.cpp`, `backend_lir_to_bir_notes_test.cpp`,
`backend_prepared_printer_test.cpp`, and
`backend_x86_prepared_handoff_label_authority_test.cpp`.
Updated only the matching source paths in `tests/backend/CMakeLists.txt`; test
names, labels, and behavior were left unchanged.

## Suggested Next

Next Step 2 mechanical move packet: create the MIR backend test tree and move
only the obvious live AArch64/MIR-owned C++ unit/model/printer/record tests,
the `backend_aarch64_*_test.cpp` files. Update only the corresponding source
paths in `tests/backend/CMakeLists.txt`; leave CLI dump tests, backend route
tests, case fixtures, disabled MIR placeholders, and legacy x86 handoff/route
debug tests for later packets.

## Watchouts

- The current label set cannot independently select BIR vs MIR yet; all
  backend tests share broad `backend` plus ad hoc `cpp`/`cli`/`aarch64`/route
  labels.
- Keep the next packet mechanical: do not move AArch64 CLI smoke tests or case
  fixtures with the C++ unit/model/printer/record tests.
- Disabled MIR dump/trace placeholders are still a pruning concern, not a move
  target.
- `return_add.c` remains a shared fixture even though one AArch64 smoke test
  uses it.

## Proof

Delegated proof command:
`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "^(backend_prepare_|backend_lir_to_bir_notes|backend_prepared_printer|backend_x86_prepared_handoff_label_authority)" --output-on-failure' > test_after.log 2>&1`

Result: passed. The build completed, CMake regenerated against the moved
sources, and the focused CTest subset passed 10/10 tests. Proof log path is
`test_after.log`.
