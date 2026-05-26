Status: Active
Source Idea Path: ideas/open/35_aarch64_calls_foldback_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fold Private Calls Helpers Into `calls.cpp`

# Current Packet

## Just Finished

Step 2 - Fold Private Calls Helpers Into `calls.cpp` completed the remaining
`calls_common.cpp` foldback packet:

- Moved `outgoing_stack_argument_bytes`,
  `outgoing_stack_argument_base_register`, and
  `scalar_integer_register_view_from_size` definitions into `calls.cpp`.
- Removed `calls_common.cpp` from `src/backend/CMakeLists.txt` and deleted the
  now-empty translation unit.
- Preserved the existing declarations in `calls.hpp` for calls-family consumers
  in `calls_moves.cpp` and `calls_dispatch_bridge.cpp`.
- Kept behavior, diagnostics, ABI semantics, and test expectations unchanged.

## Suggested Next

Next packet: supervisor should review whether Step 2 is exhausted now that
`calls_common.cpp` has been removed from the build and deleted.

## Watchouts

The remaining declarations in `calls.hpp` are still needed by external
calls-family consumers. Do not fold dispatch-facing APIs such as
`make_selected_call_argument_source` or `find_move_bundle` as part of this
packet.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch|backend_aarch64_prepared_handoff_gate|backend_aarch64_prepared_register_conversion)$' | tee test_after.log`

`test_after.log` is the proof log.
