Status: Active
Source Idea Path: ideas/open/35_aarch64_calls_foldback_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fold Private Calls Helpers Into `calls.cpp`

# Current Packet

## Just Finished

Step 2 - Fold Private Calls Helpers Into `calls.cpp` completed the first
mechanical fold-back packet for byval helpers:

- Moved `is_aarch64_byval_register_lane_move` and
  `make_byval_register_lane_prepared_source` from
  `calls_byval_aggregates.cpp` into the anonymous namespace in
  `calls_moves.cpp`.
- Removed both helper declarations from `calls.hpp`.
- Removed `calls_byval_aggregates.cpp` from `src/backend/CMakeLists.txt` and
  deleted the obsolete source file.
- Kept behavior, byval source selection, call planning, diagnostics, and test
  expectations unchanged.

## Suggested Next

Next fold-back packet: narrow or relocate the remaining shared calls helper
surface from `calls_common.cpp` only after choosing the smallest owner boundary
for each externally consumed helper.

## Watchouts

`calls_common.cpp` helpers are still consumed across `calls.cpp`,
`calls_moves.cpp`, and `calls_dispatch_bridge.cpp`; the next packet should keep
the boundary narrow and avoid folding unrelated dispatch-facing APIs at the
same time. `make_selected_call_argument_source` and `find_move_bundle` remain
externally visible for the bridge.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch|backend_aarch64_prepared_handoff_gate|backend_aarch64_prepared_register_conversion)$' | tee test_after.log`

`test_after.log` is the proof log.
