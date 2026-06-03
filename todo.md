Status: Active
Source Idea Path: ideas/open/106_prealloc_stack_layout_slice_family_fact_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Stack Layout Proof

# Current Packet

## Just Finished

Step 4 focused proof added for the structured stack-layout slice-family and
publication contract.

Focused coverage:
- Strengthened `backend_prepare_stack_layout_test.cpp` sliced local fixture to
  assert every `%slice.src.N` / `%slice.dst.N` object publishes
  `PreparedStackSliceFamily` metadata with family identity, slice offset, and
  the retained `legacy_slot_name_compatibility` marker.
- Kept the existing sliced `base.0 + byte_offset` access assertions and added
  a malformed `%slice.src.bad` local-slot address case that must fail closed by
  publishing no prepared memory access.
- Strengthened the rooted pointer binary fixture to assert the bounded
  frame-address compatibility contract: the stack object publishes
  `frame_address_value_name` plus `legacy_frame_address_name_compatibility`,
  and the prepared frame-slot address materialization consumes that value fact.
- Strengthened the copy-coalescing fixture to prove it remains placement-only:
  the coalesced scratch object must not publish slice-family,
  aggregate-address, or frame-address publication facts.

No implementation files changed in this packet.

## Suggested Next

Execute `plan.md` Step 5: final validation and close-readiness notes for the
stack-layout slice-family/publication contract.

## Watchouts

- The proof intentionally keeps `LegacySlotNameSliceFamilyCompatibility` and
  `LegacyFrameAddressNameCompatibility` as bounded bootstraps; Step 5 should
  call these out as retained exceptions.
- No focused proof was added for a future non-legacy BIR-native slice-family
  producer because that authority surface does not exist yet.
- Keep final validation read-only outside `todo.md`/`test_after.log`.

## Proof

Passed. Ran:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: 169/169 backend tests passed.

Proof log: `test_after.log`.
