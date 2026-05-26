Status: Active
Source Idea Path: ideas/open/35_aarch64_calls_foldback_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fold Dispatch Bridge Surface Into Calls Owner API

# Current Packet

## Just Finished

Step 3 - Fold Dispatch Bridge Surface Into Calls Owner API completed the
dispatch-bridge foldback packet:

- Moved the former `calls_dispatch_bridge.cpp` implementation into
  `calls.cpp`.
- Moved the bridge declarations still needed by `dispatch.cpp` and the
  instruction dispatch test into `calls.hpp`.
- Removed `calls_dispatch_bridge.cpp` from `src/backend/CMakeLists.txt` and
  deleted the obsolete bridge source/header files.
- Replaced direct bridge-header includes in `dispatch.cpp` and
  `backend_aarch64_instruction_dispatch_test.cpp` with the calls owner API.
- Kept behavior, diagnostics, ABI semantics, and test expectations unchanged.

## Suggested Next

Next packet: supervisor should review whether Step 3 is exhausted and whether
the remaining calls owner API surface should be narrowed in a separate packet.

## Watchouts

`calls.hpp` still exposes calls owner APIs used by `dispatch.cpp` and
instruction dispatch tests. This packet intentionally did not rewrite dispatch
publication, prepared edge-copy semantics, ABI behavior, or test expectations.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch|backend_aarch64_prepared_handoff_gate|backend_aarch64_prepared_register_conversion)$' | tee test_after.log`

`test_after.log` is the proof log.
