Status: Active
Source Idea Path: ideas/open/07_aarch64_call_boundary_move_emission_only.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Reduce Byval, Frame-Slot, And Materialization Reconstruction

# Current Packet

## Just Finished

Step 4 implementation reduced duplicate AArch64 local aggregate frame-address
reconstruction in call-boundary move emission.

Removed the target-local `calls_moves.cpp` and dispatch-bridge copies of the
local aggregate frame-slot scan. Before-call moves and the dispatch bridge now
route that case through `make_frame_slot_call_argument_address_source`, which
consumes explicit `FrameSlotAddress`/prepared materialization facts when they
exist and keeps the compatibility path centralized for absent selections.

## Suggested Next

Review whether prepared call planning can publish complete source selections for
the remaining absent-selection compatibility cases, then retire the centralized
fallbacks one family at a time.

## Watchouts

- Keep source-selection authority in prepared call facts when
  `source_selection` is present.
- Do not touch the transient `review/` artifacts unless explicitly delegated.
- Treat expectation weakening or named-test shortcuts as route failures.
- Remaining temporary fallback: absent-selection local aggregate pointer
  frame-address publication still needs prepared
  `LocalFrameAddressMaterialization`/`FrameSlotAddress` source selection with
  frame slot id, byte offset, size, align, and materialization site.
- Remaining temporary fallback: absent-selection indirect byval lane handling
  still needs prepared `ByvalRegisterLane` source selection with source slot,
  stack offset, source size, align, and `byval_lane_extent_bytes`.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_aarch64_prepared_memory_operand_records|backend_aarch64_operand_resolution)$'`

Passed. Proof log: `test_after.log`.

Supervisor follow-up validation:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
  passed 162/162.
