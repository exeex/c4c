Status: Active
Source Idea Path: ideas/open/220_backend_test_tree_split_bir_mir_and_prune_legacy.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Normalize CMake Labels, Names, And Selectors

# Current Packet

## Just Finished

Step 5 selector normalization packet completed. Appended the `bir` label to
all tests registered by `tests/backend/bir/CMakeLists.txt` and the `mir` label
to all tests registered by `tests/backend/mir/CMakeLists.txt`, while preserving
the existing `backend`, `internal`, `cpp`, `cli`, `aarch64`, `x86`,
`backend_route`, and related specialized labels. Updated
`tests/backend/OWNERSHIP.md` to document `ctest -L bir` and `ctest -L mir` as
the independent backend ownership selectors.

## Suggested Next

Next Step 6 recommendation: run final validation and close-readiness review for
the backend test tree split, including BIR/MIR subset proof, backend inventory
sanity checks, and confirmation that no stale MIR dump/trace placeholders or
old flat-path references remain.

## Watchouts

- `ctest -L bir` currently selects 108 BIR-owned backend tests; `ctest -L mir`
  currently selects 27 MIR-owned backend tests.
- The ownership labels are appended from each subdirectory's registered test
  list, so future tests added under `tests/backend/bir` or `tests/backend/mir`
  inherit the matching selector without changing the shared backend helper.
- Existing labels were preserved; `backend_route`, `aarch64`, `x86`, `cpp`,
  `cli`, and external-toolchain selectors remain available.

## Proof

Delegated proof command:
`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -L "bir" --output-on-failure && ctest --test-dir build -L "mir" --output-on-failure && ctest --test-dir build -R "^backend_" --output-on-failure' > test_after.log 2>&1`

Result: passed. The default build completed, `ctest -L bir` passed 108/108
tests, `ctest -L mir` passed 27/27 tests, and the `^backend_` CTest subset
passed 135/135 tests. Proof log path is `test_after.log`.
