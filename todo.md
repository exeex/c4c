Status: Active
Source Idea Path: ideas/open/220_backend_test_tree_split_bir_mir_and_prune_legacy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create BIR Test Tree And Move BIR-Owned Tests

# Current Packet

## Just Finished

Step 2 BIR semantic-route ownership packet completed. Moved the unambiguous
x86 prepared-route C++ tests and riscv/x86 semantic-BIR codegen route test
definitions from root backend ownership into `tests/backend/bir`. Target names,
test names, labels, route snippets, and behavior were left unchanged.

## Suggested Next

Next Step 2 recommendation: inspect the remaining root backend CMake ownership
for MIR/debug-route or target-owned tests and delegate the next mechanical
ownership split only where the assertion target is unambiguous.

## Watchouts

- The BIR CLI dump/prepared-dump and BIR C++ executable/test definitions now
  live under BIR CMake ownership.
- The x86 handoff-boundary and route-debug source files now live under
  `tests/backend/bir/` because their assertions inspect semantic BIR,
  prepared-BIR, or prepared route-debug surfaces.
- The riscv/x86 `backend_codegen_route_*` semantic-BIR tests now live under
  BIR CMake ownership; root backend CMake no longer has `semantic_bir`,
  `dump_bir`, `dump_prepared_bir`, x86 handoff-boundary, or x86 route-debug
  registrations.
- Do not move AArch64 tests, case fixtures, disabled MIR placeholders, or
  implementation files as part of Step 2.
- Disabled MIR dump/trace placeholders are still a pruning concern, not a move
  target.

## Proof

Delegated proof command:
`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "^(backend_x86_handoff_boundary|backend_x86_route_debug|backend_codegen_route_x86_64_|backend_codegen_route_riscv64_|backend_cli_dump_bir|backend_cli_dump_prepared_bir)" --output-on-failure' > test_after.log 2>&1`

Result: passed. CMake regenerated, the moved x86 route executables built from
`tests/backend/bir`, and the focused CTest subset passed 95/95 tests. Proof log
path is `test_after.log`.
