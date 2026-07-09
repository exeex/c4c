Status: Active
Source Idea Path: ideas/open/630_string_constant_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Reclassify Rows After String-Label Pointer Authority

# Current Packet

## Just Finished

Completed Step 7: implemented narrow prepared/RV64 separation between
string-label pointer materialization authority and real string-byte range
authority.

Concrete changes:

- Added `MemoryLayoutAuthorityKind::StringConstantLabelPointer` plus
  `prepared_string_constant_label_pointer_has_authority`.
- `publish_string_constant_local_memory_authority` now grants
  `StringConstantLabelPointer` only for default-address-space, nonvolatile,
  pointer-result string-constant accesses with unambiguous identity, label
  spelling, `byte_offset=0`, `size_bytes=8`, `align_bytes=8`, and a complete
  string identity/extent record. This bypasses the 8-byte string-byte range
  proof only for pointer materialization.
- Existing `StringConstantBytes` authority remains range-proven against the
  string byte extent; non-pointer 8-byte string-byte reads from short strings
  still become `ProvenOutOfBounds` with unknown layout authority.
- RV64 prepared local-memory string pointer emission and diagnostics now
  consume the new label-pointer authority rather than byte authority.
- Focused coverage now proves short and empty string pointer materializations,
  rejects missing/ambiguous identity, non-default address space, volatile
  access, wrong pointer size/alignment, missing label, byte-result loads, and
  wrong frame/global/pointer owner bases.

## Suggested Next

Execute Step 8 as a row reclassification packet: rerun the idea-630 affected
rows that were previously blocked only by short-string
`proven_out_of_bounds` pointer materializations and reclassify them against
the new `StringConstantLabelPointer` authority. Split any remaining failures
by owner family rather than expanding this Step 7 route; expected separate
families remain direct-global, aggregate/block-entry stack-home publication,
pointer-value byte access, select-carrier publication, and prepared
move-bundle fan-in.

## Watchouts

The prepared construction grants `StringConstantLabelPointer` to eligible
`LoadLocalInst` and `LoadGlobalInst` string-constant pointer-result accesses,
but the owned RV64 emission slice only had an existing local-memory
`LoadLocalInst` consumer to update. If Step 8 finds a `LoadGlobalInst` pointer
row still rejected at RV64 emission, split that as a consumer-support packet
rather than weakening global-data byte authority. The trailing-NUL extent
mismatch from Step 6 is still separate: byte/char accesses near a string
terminator should remain on `StringConstantBytes` and should not inherit
label-pointer authority.

## Proof

Ran the supervisor-selected proof command exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_riscv_object_emission)$' > test_after.log 2>&1`

Result: passed. `test_after.log` records both
`backend_prepare_stack_layout` and `backend_riscv_object_emission` passing.
