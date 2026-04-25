Status: Active
Source Idea Path: ideas/open/07_bir-local-gep-family-extraction.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory The Local GEP Family

# Current Packet

## Just Finished

Step 1 - Inventory The Local GEP Family completed a read-only boundary
inventory for `src/backend/bir/lir_to_bir/memory/local_slots.cpp`.

Local GEP functions found in `local_slots.cpp`:

- `resolve_local_aggregate_gep_slot` at line 316: constant local aggregate GEP
  to a scalar leaf slot, including raw `i8` byte-slice leaf handling.
- `resolve_local_aggregate_pointer_array_slots` at line 412: local aggregate
  GEP to a pointer array slot list.
- `resolve_local_aggregate_dynamic_pointer_array_access` at line 525: local
  aggregate GEP with a dynamic array index that selects pointer leaves.
- `resolve_local_aggregate_gep_target` at line 667: local aggregate GEP to
  type text plus byte offset, used before deciding scalar leaf vs aggregate
  subobject handling.
- `resolve_local_dynamic_aggregate_array_access` at line 760: local aggregate
  GEP with a dynamic index over a repeated aggregate extent.
- `build_dynamic_local_aggregate_array_access` at line 812: builds the dynamic
  local aggregate access descriptor from a scalar local-slot pointer base.
- `resolve_dynamic_local_aggregate_gep_projection` at line 840: projects a GEP
  through a previously recorded dynamic local aggregate access.
- `try_lower_dynamic_local_aggregate_gep_projection` at line 868: map-facing
  wrapper for dynamic local aggregate GEP projection.
- `try_lower_local_slot_pointer_gep` at line 890: relative GEP from a tracked
  local slot pointer value.
- `try_lower_local_array_slot_gep` at line 976: GEP from a direct local scalar
  array allocation.
- `try_lower_local_pointer_array_base_gep` at line 1031: GEP from a tracked
  local pointer array base, including dynamic pointer/scalar array publication.
- `try_lower_local_pointer_slot_base_gep` at line 1202: one-index GEP from an
  exact local pointer slot, including local array and local aggregate bases.

File-private helpers/structs that appear exclusive to the local aggregate GEP
family:

- `LocalAggregateRawByteSliceLeaf` at line 101.
- `resolve_local_aggregate_raw_byte_slice_leaf` at line 106.

Nearby helpers/includes that must remain available to `local_slots.cpp` unless
later edits also move or rewrite their non-GEP users:

- `resolve_scalar_layout_facts_at_byte_offset` at line 83 must remain exported
  through `memory_helpers.hpp`; `memory/provenance.cpp` calls it directly.
- `resolve_scalar_layout_leaf_facts_at_byte_offset` at line 24 is the private
  implementation helper for `resolve_scalar_layout_facts_at_byte_offset`.
- `collect_local_scalar_array_slots` at line 165 is used by
  `memory/addressing.cpp` and by local store/load state recording in
  `local_slots.cpp`.
- `is_local_array_element_slot` at line 202 is used by
  `try_lower_local_slot_store` and `try_lower_tracked_local_pointer_slot_load`.
- `parse_local_array_type` at line 215 belongs with
  `lower_local_memory_alloca_inst`, not the GEP move.
- Type aliases at lines 13-15 are not exclusively GEP-owned:
  `DynamicLocalAggregateArrayAccess` is also used by dynamic local aggregate
  load/store helpers after line 2302; `LocalAggregateGepTarget` and
  `DynamicLocalPointerArrayAccess` are convenient for the GEP cluster but
  should be handled deliberately during extraction.
- Current includes should not be removed from `local_slots.cpp` as part of a
  pure GEP move without rechecking remaining users: `../lowering.hpp`,
  `memory_helpers.hpp`, `<algorithm>`, `<optional>`, `<string>`,
  `<string_view>`, `<utility>`, and `<vector>` all have non-GEP or shared
  users in the file.

## Suggested Next

Extract only the listed local GEP cluster plus the exclusive raw-byte-slice
helper into the planned local GEP unit, then adjust declarations/includes
without moving `plan.md` or changing behavior.

## Watchouts

`build_dynamic_local_aggregate_array_access` is a boundary-sensitive helper:
it is GEP-created state, but dynamic local aggregate load/store later consume
the same access type. Keep the public/static declaration stable unless the next
packet intentionally updates all users. The root has an unrelated untracked
`ideas/open/08_bir-address-projection-model-consolidation.md`; this packet did
not inspect or modify it.

## Proof

No build was required or run for this read-only inventory packet. Per packet
instruction, `test_before.log` and `test_after.log` were not created or
modified.
