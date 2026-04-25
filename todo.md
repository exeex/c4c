Status: Active
Source Idea Path: ideas/open/08_bir-address-projection-model-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Projection Paths
你該做code review了
你該做test baseline review了

# Current Packet

## Just Finished

Step 1 inventory mapped the repeated address projection facts across local GEP,
global/relative GEP, and provenance scalar subobject paths without editing
implementation files.

Repeated facts found:

- `addressing.cpp` and `local_gep.cpp` both walk `AggregateTypeLayout` with the
  same core state: trimmed current type, parsed/resolved GEP index, current byte
  offset, array element layout/stride, struct field byte offset, and final leaf
  layout/type.
- `resolve_global_gep_address`, `resolve_relative_global_gep_address`,
  `resolve_relative_gep_target`, `resolve_local_aggregate_gep_slot`, and
  `resolve_local_aggregate_gep_target` duplicate the same array/struct
  projection step: reject invalid or zero-sized layouts, reject out-of-range
  array/field indices, add `index * element_layout.size_bytes` or
  `field.byte_offset`, and update `current_type`.
- Dynamic local/global aggregate paths repeat extent discovery around
  `find_repeated_aggregate_extent_at_offset`: compute the repeated element
  count, stride, and base byte offset, then populate
  `DynamicLocalAggregateArrayAccess`, `DynamicGlobalAggregateArrayAccess`, or
  scalar/pointer dynamic access records.
- Scalar leaf/subobject checks are split between
  `resolve_scalar_layout_facts_at_byte_offset`, local raw `i8` GEP handling in
  `resolve_local_aggregate_raw_byte_slice_leaf`, and provenance
  `can_address_scalar_subobject`; each needs object size, scalar leaf type,
  scalar leaf byte offset, and access-size containment.
- Local slot policy repeatedly revalidates the same semantic leaf fact through
  `aggregate_slots.leaf_slots.find(byte_offset)`, while global/provenance paths
  need the same byte offset and leaf type facts but publish different state.

Shared semantic facts to consolidate:

- Absolute or relative projected byte offset after each GEP step.
- Child projection kind (`ArrayElement` or `StructField`), child index, child
  type text, child layout, child byte offset, and element/field stride source.
- Repeated extent facts: base byte offset, element type text, element count,
  element stride bytes, and whether struct-field runs are allowed.
- Scalar leaf facts: object size, scalar leaf type text, BIR type, scalar leaf
  size, scalar leaf byte offset, and whether an access type fits within the
  enclosing object.

Caller-specific policy to keep at call sites:

- Local GEP decides whether a projected byte offset maps to
  `local_pointer_slots`, `local_slot_pointer_values`, `local_pointer_array_bases`,
  `dynamic_local_pointer_arrays`, or `dynamic_local_aggregate_arrays`.
- Global/relative GEP decides whether first-index policy is strict global-base
  zero, relative-base stride addition, dynamic pointer array, dynamic aggregate
  array, or dynamic scalar array.
- Provenance decides opaque pointer base allowances, scratch-slot emission,
  pointer-value address publication, and load/store acceptance.
- Raw `i8` byte-slice support stays policy-specific: local aggregate raw GEP
  requires a leaf slot at the absolute byte offset, while generic pointer raw
  `i8` GEP may emit pointer addition for runtime pointers.

## Suggested Next

Implement Step 2 by extending existing helper/result surfaces before replacing
call-site logic. First target files:

- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`: extend
  `AggregateByteOffsetProjection` or add a small adjacent result type for
  reusable projection-step facts, and consider adding scalar access-fit facts to
  `ScalarLayoutByteOffsetFacts` instead of creating a new header.
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`: extend only existing
  dynamic/access structs if a caller needs an already-computed repeated extent
  or scalar leaf fact carried forward.
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`: central helper
  implementation is already the owner of `resolve_aggregate_byte_offset_projection`,
  `find_repeated_aggregate_extent_at_offset_impl`, and global/relative GEP
  projection callers.
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`: owns
  `resolve_scalar_layout_facts_at_byte_offset`; use it for scalar leaf/access
  fact strengthening.
- `src/backend/bir/lir_to_bir/memory/local_gep.cpp` and
  `src/backend/bir/lir_to_bir/memory/provenance.cpp`: first consumers after the
  helper result facts exist.

## Watchouts

- Preserve BIR output, diagnostics, and testcase expectations.
- Keep `BirFunctionLowerer` as the stateful lowering owner.
- Prefer existing `memory_types.hpp` and `memory_helpers.hpp`; do not add a new
  header unless the active plan is explicitly revised.
- Do not turn the inventory into testcase-shaped matching.
- Do not move publication policy into pure helpers. Helpers should report facts;
  callers should still decide which state map to update.
- Be careful with first-index policy: local aggregate GEP, absolute global GEP,
  relative global GEP, dynamic aggregate GEP, and runtime pointer provenance use
  similar layout math but different acceptance rules.
- `resolve_scalar_layout_facts_at_byte_offset` currently reports object size and
  optional scalar leaf, while provenance `can_address_scalar_subobject` only
  uses object-size containment when `stored_type == Void`. If Step 2 strengthens
  scalar facts, preserve the existing stored-type and opaque-pointer shortcuts.

## Proof

No build required for this read-only inventory packet. No test logs were
created or modified.
