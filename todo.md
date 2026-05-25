Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Retire Obsolete Calls Translation Units

# Current Packet

## Just Finished

Step 4 retired `calls_printing.cpp`. Its remaining live public instruction
record constructors and selection-status helpers moved into `calls.cpp`, and
the obsolete translation unit was removed from `src/backend/CMakeLists.txt`.

No signature-metadata source entry needed removal because
`backend_aarch64_signature_metadata_test.cpp` already audits `calls.cpp` and
`calls.hpp` rather than the retired split file.

## Suggested Next

Have the supervisor review and commit the Step 4 retirement slice, then decide
whether the active runbook is exhausted or needs lifecycle review for any
remaining calls-emission consolidation work.

## Watchouts

- `calls_moves.cpp` still owns the immediate-cast call-argument publication
  emitter that is actually used by move lowering. The duplicate stale copy in
  the retired file was not moved.
- `clang-format` is not installed in this environment; the moved code was
  build-checked but not auto-formatted.
- Do not claim further Step 4 retirement without checking CMake and signature
  metadata together.

## Proof

Ran the delegated proof command:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } | tee test_after.log'`

Result: passed. The build completed and CTest reported 162/162 `^backend_`
tests passed.

Proof log: `test_after.log`.
