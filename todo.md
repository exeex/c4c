Status: Active
Source Idea Path: ideas/open/106_prealloc_stack_layout_slice_family_fact_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Narrow The Contract

# Current Packet

## Just Finished

Step 3 implementation completed for the structured stack-layout
slice-family/publication boundary.

Changed routes:
- Added `PreparedStackSliceFamily` on `PreparedStackObject`, carrying prepared
  family/root identity, slice offset, and a
  `legacy_slot_name_compatibility` marker.
- Isolated suffix parsing as `legacy_slot_name_slice_family_compatibility`.
  The producer copies the prepared slot spelling before interning family names,
  so the compatibility bootstrap does not read from invalidated name-table
  storage.
- Routed `slot_assignment.cpp` fixed-family grouping, fixed-location checks,
  and offset-preserving slice placement through `object.slice_family` instead
  of reparsing prepared slot names.
- Routed coordinator direct frame-slot slice coverage through structured
  `FrameSlotPublicationFacts::slice_coverage_by_family`, built from stack
  object metadata plus physical frame slots. Frame-slot IDs and offsets remain
  prealloc placement facts.
- Routed `find_prepared_stack_frame_offset_by_name` candidate lookup through
  object slice-family metadata. It still parses the external requested name
  only to interpret the query.
- Added `aggregate_address_published` on stack objects before aggregate
  address exposure mutates `requires_home_slot` and `permanent_home_slot`.
- Added `frame_address_value_name` and
  `legacy_frame_address_name_compatibility` on stack objects. The retained
  result-name/slot-name equality path now publishes this explicit object fact
  first; `append_frame_slot_address_materialization` consumes only that fact.

Retained boundaries:
- Physical frame placement, object ordering, frame size/alignment, and
  copy/alloca/regalloc home-slot hints remain in prealloc.
- `LegacySlotNameSliceFamilyCompatibility` remains as the narrow bootstrap for
  current scalarized aggregate slot names.
- `LegacyFrameAddressNameCompatibility` remains as the narrow bootstrap for
  current local frame-address values until a broader explicit BIR fact exists.
- No implementation changes were made to unrelated alloca/copy coalescing
  behavior, pointer carriers, BIR lowering, or MIR codegen.

## Suggested Next

Execute `plan.md` Step 4: add focused proof for the structured slice-family
and publication boundary, especially malformed/unstructured fail-closed cases
and retained compatibility bounds.

## Watchouts

- Step 4 should prove that consumers now read structured facts, not suffix
  parsing/name equality directly.
- The retained compatibility paths need explicit bounds: malformed slice names
  should fail closed, and frame-address name equality should be treated as a
  named bootstrap rather than primary semantic authority.
- Keep alloca/copy coalescing as placement-only proof; do not broaden their
  implementation in the proof packet.

## Proof

Passed. Ran:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: 169/169 backend tests passed.

Proof log: `test_after.log`.
