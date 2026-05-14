Status: Active
Source Idea Path: ideas/open/220_backend_test_tree_split_bir_mir_and_prune_legacy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Classify Fixtures And Prune Legacy Route Debris

# Current Packet

## Just Finished

Step 4 fixture and legacy cleanup packet completed. Added
`tests/backend/OWNERSHIP.md` with auditable `bir-live`, `mir-live`,
`shared-fixture`, `legacy`, `deprecated`, and `delete` categories for backend
tests and case fixtures. Removed the disabled root
`backend_cli_dump_mir_*`/`backend_cli_trace_mir_*` x86 debug-route CMake
placeholders and recorded each deletion reason in the inventory. Live AArch64
MIR smoke tests and all `tests/backend/case/*.c` files remain.

## Suggested Next

Next Step 5 recommendation: normalize CMake labels/selectors so BIR and MIR
backend subsets can be selected independently, without moving fixture files or
changing backend behavior.

## Watchouts

- `tests/backend/OWNERSHIP.md` classifies unreferenced case files as `legacy`
  source corpus inventory, not active backend proof.
- The deleted MIR dump/trace placeholders were disabled x86/debug route tests;
  their CMake names now appear only in the inventory delete list.
- Existing labels were intentionally preserved for this packet. Independent
  `bir` and `mir` label/selectors remain a Step 5 concern.

## Proof

Delegated proof command:
`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "^backend_" --output-on-failure && ! ctest --test-dir build -N -R "backend_cli_(dump|trace)_mir" | rg "backend_cli_(dump|trace)_mir"' > test_after.log 2>&1`

Result: passed. The default build completed, the `^backend_` CTest subset
passed 135/135 tests, and the negative CTest listing check found no remaining
`backend_cli_dump_mir_*` or `backend_cli_trace_mir_*` registrations. Proof log
path is `test_after.log`.
