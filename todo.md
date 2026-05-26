Status: Active
Source Idea Path: ideas/open/35_aarch64_calls_foldback_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fold Private Calls Helpers Into `calls.cpp`

# Current Packet

## Just Finished

Step 2 - Fold Private Calls Helpers Into `calls.cpp` completed the
`calls_moves.cpp` translation-unit foldback packet:

- Mechanically moved the former `calls_moves.cpp` implementation into
  `calls.cpp`.
- Removed `calls_moves.cpp` from `src/backend/CMakeLists.txt` and deleted the
  obsolete translation unit.
- Preserved existing public declarations in `calls.hpp` for dispatch and bridge
  callers.
- Kept behavior, diagnostics, ABI semantics, and test expectations unchanged.

## Suggested Next

Next packet: supervisor should decide whether Step 2 is exhausted or whether
remaining dispatch/bridge-owned calls declarations need a separate plan step or
review before further foldback.

## Watchouts

This packet intentionally did not fold dispatch bridge implementation or alter
ABI/prepare semantics. `calls.hpp` still exposes declarations consumed by
dispatch and bridge code; avoid deleting those without a dedicated ownership
check.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch|backend_aarch64_prepared_handoff_gate|backend_aarch64_prepared_register_conversion)$' | tee test_after.log`

`test_after.log` is the proof log.
