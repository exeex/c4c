Status: Active
Source Idea Path: ideas/open/268_aarch64_intrinsics_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create The Intrinsics Owner Boundary

# Current Packet

## Just Finished

Completed Step 2 from `plan.md`: created
`src/backend/mir/aarch64/codegen/intrinsics.cpp` and
`src/backend/mir/aarch64/codegen/intrinsics.hpp` as the compiled AArch64
intrinsics owner boundary.

Moved current accepted intrinsic ownership out of broad owners without adding
new semantics:

- `dispatch.cpp` now routes through the intrinsics owner for prepared carrier
  presence and lowering.
- `intrinsics.cpp` owns prepared carrier lookup, intrinsic diagnostics, current
  record construction, selected-status checks, machine-node construction, and
  printer helpers for `ScalarFpUnary/FAbs`, `Crc32W`, `VectorLoad`, and
  `VectorAdd`.
- `machine_printer.cpp` delegates intrinsic payload printing to the intrinsics
  owner.
- `instruction.cpp` no longer owns intrinsic record construction or selection
  status logic; `instruction.hpp` keeps the existing public factory
  declarations for current callers and tests.
- `src/backend/CMakeLists.txt` now builds `intrinsics.cpp`.

## Suggested Next

Supervisor should review the Step 2 ownership diff for route fit and commit
readiness. A coherent next implementation packet, if accepted, should continue
with the next runbook step rather than expanding the intrinsic surface inside
Step 2.

## Watchouts

- Unsupported/deferred intrinsic families remain fail-closed; no tests or
  expectations were changed.
- Existing callers still include `instruction.hpp` for intrinsic factory
  declarations; implementation ownership moved to `intrinsics.cpp`.
- `test_after.log` contains the final delegated proof run.

## Proof

Ran delegated proof:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`.

Result: passed. `test_after.log` records a successful build and `139/139`
backend tests passing.
