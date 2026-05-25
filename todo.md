Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consolidate The Local-Frame Helper Boundary

# Current Packet

## Just Finished

Step 3 consolidated the local-frame helper boundary after the retained
`CallInst` reconstruction helper chain was deleted from
`src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`.

- Confirmed the obsolete local-frame publication helpers that re-read
  `CallInst::arg_types`, `CallInst::arg_abi`, or callee spelling are absent
  from `calls_argument_sources.cpp` and `calls.hpp`.
- Confirmed the surviving `make_frame_slot_call_argument_address_source` and
  `make_local_frame_address_call_argument_source` declarations remain
  emission-only boundaries: callers pass prepared argument/home state, and the
  helpers only build prepared `MemoryOperand` sources.
- No code or build-metadata edit was needed for Step 3 because no obsolete
  local-frame metadata-reconstruction prototype survived the Step 2 deletion.

## Suggested Next

Supervisor should choose whether to run Step 4 broader backend checkpoint or
route lifecycle handling for the active plan.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not weaken tests or mark a nearby publication case unsupported to claim
  progress.
- The surviving `make_*local_frame*call_argument*source` helpers are still used
  by `calls_moves.cpp` and `calls_preservation.cpp`; removing their declarations
  would cross from boundary closeout into unrelated emission reshaping.
- `rg -n "call_argument_allows_local_frame_address_publication|call_argument_is_pointer|call_argument_is_byval_copy|call_argument_call|arg_abi|arg_types|local_frame_address_name_matches|make_local_frame_address_call_argument_source|make_frame_slot_call_argument_address_source|allows_local_aggregate_address_publication" src/backend/mir/aarch64/codegen/calls_argument_sources.cpp src/backend/mir/aarch64/codegen/calls.hpp src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp src/backend/mir/aarch64/codegen/calls_moves.cpp src/backend/mir/aarch64/codegen/calls_preservation.cpp`
  shows no obsolete retained-`CallInst` local-frame helper chain in
  `calls_argument_sources.cpp` or `calls.hpp`; only prepared-fact gates and
  live emission helper uses remain.

## Proof

Step 3 delegated proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.

Proof log: `test_after.log` contains `100% tests passed, 0 tests failed out of 162`.
