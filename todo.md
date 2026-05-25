Status: Active
Source Idea Path: ideas/open/07_aarch64_call_boundary_move_emission_only.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate Emission-Only Boundary

# Current Packet

## Just Finished

Step 5 repaired the Step 3 AArch64 prepared source-selection regression
exposed by `c_testsuite_aarch64_backend_src_00216_c` and
`c_testsuite_aarch64_backend_src_00204_c`.

The source path changed in Step 3 when explicit call-argument
`source_selection` routing stopped letting `PriorPreservation` and
`FrameSlotValue` selections use the legacy local aggregate frame-address
publication path. That made `00216.c` publish preserved aggregate bytes as a
pointer instead of publishing the aggregate address. Step 3 also made selected
`ByvalRegisterLane` sources fatal before consulting prepared payload stores;
`00204.c` has stack byval arguments whose selected slot records only the first
8-byte lane while prepared payload stores contain the full 12/13-byte stack
argument payload.

The repair keeps explicit prepared source authority for
`FrameSlotAddress`, `LocalFrameAddressMaterialization`, and
`ByvalRegisterLane`, while restoring the old compatibility behavior only where
the prepared facts are incomplete but payload/address facts already exist:
`PriorPreservation`/`FrameSlotValue` may still publish a local aggregate address
for pointer frame-slot arguments, and selected byval lane emission may fall back
to prepared payload stores when the selected source slot is narrower than the
lane extent.

## Suggested Next

Continue Step 5 validation with the supervisor-selected broader acceptance
scope, or retire the remaining documented absent-selection fallbacks in a
separate packet if validation remains green.

## Watchouts

- Keep source-selection authority in prepared call facts when
  `source_selection` is present.
- Do not touch the transient `review/` artifacts unless explicitly delegated.
- Treat expectation weakening or named-test shortcuts as route failures.
- Do not re-tighten selected byval lane fallback until prepared records provide
  full selected source bytes for stack byval lanes wider than the first 8-byte
  slot.
- Do not remove the `PriorPreservation`/`FrameSlotValue` local aggregate
  address compatibility path until prepared source selections distinguish
  preserved value bytes from required pointer/address publication.
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

Step 5 regression repair proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00216_c|backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_aarch64_prepared_memory_operand_records|backend_prepare_frame_stack_call_contract)$'`

Passed 6/6. Proof log: `test_after.log`.

Supervisor follow-up validation:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_'`
  passed 220/220, including `00204.c` and `00216.c`.
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
  passed 162/162.
- Backend regression guard passed with `test_before.log` and `test_after.log`:
  162 passed before, 162 passed after, no new failures.
