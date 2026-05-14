Status: Active
Source Idea Path: ideas/open/220_backend_test_tree_split_bir_mir_and_prune_legacy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Create MIR Test Tree And Move MIR-Owned Tests

# Current Packet

## Just Finished

Step 3 first MIR ownership packet completed. Created `tests/backend/mir`,
moved the current unambiguous AArch64 MIR-owned C++ unit/model tests there,
and moved the live AArch64 `.c -> .s` smoke/failure CMake registrations under
MIR CMake ownership. Target names, test names, labels, snippets, and behavior
were preserved; only source path ownership changed.

## Suggested Next

Next Step 3 or Step 4 recommendation: inspect the remaining root MIR dump/trace
CLI placeholders and backend case fixtures, then delegate either a narrow MIR
debug-route ownership packet or the fixture/legacy classification packet only
where ownership is unambiguous.

## Watchouts

- `tests/backend/CMakeLists.txt` now delegates AArch64 MIR test ownership to
  `tests/backend/mir/CMakeLists.txt`; root backend CMake no longer directly
  owns `backend_aarch64_*` or `backend_cli_aarch64_asm_*` registrations.
- The two AArch64 `.c` files stayed in `tests/backend/case/` as fixture inputs,
  not proof ownership moves.
- Existing labels were intentionally preserved for this packet. Independent
  `mir` label/selectors remain a Step 5 normalization concern.
- Root `backend_cli_dump_mir_*` and `backend_cli_trace_mir_*` x86 debug-route
  placeholders were left untouched; their ownership/pruning still needs a
  separate classification packet.

## Proof

Delegated proof command:
`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "^(backend_aarch64_|backend_cli_aarch64_asm_)" --output-on-failure' > test_after.log 2>&1`

Result: passed. CMake regenerated, the moved AArch64 executables built from
`tests/backend/mir`, and the focused CTest subset passed 27/27 tests. Proof log
path is `test_after.log`.
