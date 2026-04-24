Status: Active
Source Idea Path: ideas/open/02_bir-memory-helper-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Equivalent Helpers

# Current Packet

## Just Finished

Completed `Step 1: Inventory Equivalent Helpers` as a read-only inventory pass.
AST-backed signature/caller checks covered `memory/addressing.cpp`,
`memory/provenance.cpp`, `memory/local_slots.cpp`, and
`memory/memory_helpers.hpp`; targeted source reads filled in policy details.

Concrete helper families to merge:

- Scalar leaf and scalar subobject layout facts across
  `memory/local_slots.cpp` and `memory/provenance.cpp`.
  `resolve_scalar_leaf_type_at_byte_offset(...)` and
  `can_address_scalar_subobject(...)` both walk aggregate layout from a byte
  offset through arrays/structs and need scalar leaf facts. Merge the common
  "layout leaf at byte offset" traversal behind `memory_helpers.hpp`; keep
  access-size, `stored_type`, byte-wise scalar object representation, and
  `allow_opaque_ptr_base` policy in provenance callers.
- Repeated aggregate extent and pointer-array extent traversal across
  `memory/addressing.cpp` and `memory/local_slots.cpp`.
  `find_repeated_aggregate_extent_at_offset_impl(...)`,
  `find_pointer_array_length_at_offset(...)`, and the local-slot callers that
  consume repeated extents all share recursive array/struct byte-offset
  descent. Merge the traversal/projection core behind `memory_helpers.hpp`;
  preserve separate public helpers for repeated aggregate extents, nested-only
  extents, and pointer-array length because their return shapes and acceptance
  policy differ.
- Byte-storage reinterpretation checks across `memory/addressing.cpp` and
  `memory/local_slots.cpp`.
  `can_reinterpret_byte_storage_view(...)` is already shared but remains
  implemented in `addressing.cpp`; move/normalize it behind
  `memory_helpers.hpp` so local aggregate GEP slot/target and pointer-array
  projection paths use the same pure helper surface.

Intentionally distinct for now:

- `resolve_relative_gep_target(...)`, `resolve_global_gep_address(...)`,
  `resolve_relative_global_gep_address(...)`,
  `resolve_local_aggregate_gep_slot(...)`, and
  `resolve_local_aggregate_gep_target(...)` look structurally similar but
  encode different base-index, signedness, byte-storage reinterpretation, and
  return-shape policy. Do not merge them in Step 2.
- Dynamic pointer/aggregate array projections in `addressing.cpp` and
  `local_slots.cpp` share traversal mechanics, but they also lower dynamic
  indexes, collect slots, and write global/local-specific result records.
  Step 3 may extract only common traversal/projection facts, not map updates
  or dynamic result construction.
- Local `memcpy`/`memset` leaf-view helpers in `local_slots.cpp` duplicate
  sorted-leaf collection and local-array-view resolution, but they carry
  caller-specific fill/copy sizing and value materialization policy. Leave
  these local unless a later packet extracts a pure leaf enumeration helper
  without changing memop behavior.

## Suggested Next

Execute `Step 2: Merge Scalar Leaf And Scalar Subobject Reasoning`.
Suggested merge order:

1. Add a pure scalar leaf fact/result helper declaration in
   `memory/memory_helpers.hpp`.
2. Reuse it from `memory/local_slots.cpp` for raw byte-slice scalar leaf lookup.
3. Reuse it from `memory/provenance.cpp` inside
   `can_address_scalar_subobject(...)`, keeping provenance-specific access
   policy local.
4. Run the delegated backend build and `git diff --check`.

## Watchouts

- Do not create additional `.hpp` files.
- Do not split `coordinator.cpp`.
- Keep `BirFunctionLowerer` as the memory state owner.
- Keep shared helpers pure and argument-driven behind `memory_helpers.hpp`.
- Preserve behavior; do not rewrite expectations as proof.
- For Step 3, merge repeated aggregate and pointer-array offset descent only
  where array/struct traversal semantics match. Keep include-struct-field-runs,
  pointer-element requirements, ambiguity/fallback behavior, and dynamic
  result construction caller-visible.
- For Step 4, normalize `can_reinterpret_byte_storage_view(...)`; do not fold
  it into broader GEP helpers or move provenance/map mutation into shared
  helpers.

## Proof

No build required; this was a read-only inventory plus `todo.md` update packet.
Proof command: `git diff --check -- todo.md`.
Proof log: not applicable for this no-build packet; no `test_after.log`
regenerated.
