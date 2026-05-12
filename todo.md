Status: Active
Source Idea Path: ideas/open/194_bir_global_memory_provenance_linknameid_expansion.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Or Tighten LinkNameId Authority For The Selected Route

# Current Packet

## Just Finished

Completed Step 1 inventory for the remaining global memory/provenance routes
and selected one non-187 route for `LinkNameId` hardening.

Inventory findings:
- `src/backend/bir/lir_to_bir/memory/provenance.cpp` still has direct
  global load/store paths that derive emitted `global_name_id` by
  `global_types.find(global_name)`. These paths are spelling-first at the LIR
  operand boundary, but the resulting BIR `LoadGlobalInst` and
  `StoreGlobalInst` carry `global_name_id`; the raw string should be treated as
  a compatibility lookup only when no structured id is available.
- The idea 187 addressed-global pointer route is already handled by
  `GlobalPointerSlotKey{link_name_id, global_name, byte_offset}` and the
  `AddressedGlobalPointerSlots` / `AddressedGlobalPointerValueSlots` maps.
  Keep it as comparison context only; do not claim idea 194 progress by
  re-proving this route.
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp` documents the
  function-local global pointer/provenance maps as route-local handles. The
  string keys in `GlobalPointerMap`, `GlobalObjectPointerMap`,
  `GlobalAddressIntMap`, `GlobalObjectAddressIntMap`, `GlobalAddressSlots`,
  and dynamic global maps are lowering scratch names or final spellings inside
  one function; they should not be treated as module semantic identity when a
  `LinkNameId` is present.
- `src/backend/bir/lir_to_bir/memory/value_materialization.cpp` materializes
  `DynamicGlobalScalarArrayAccess` loads with `access.global_name` and
  re-derives `global_name_id` through `link_name_id_for_dynamic_global(...)`.
  Existing coverage proves drifted display names still emit the canonical id,
  but this route has no dedicated stale/missing-id fail-closed test yet.
- `src/backend/bir/lir_to_bir/global_initializers.cpp`,
  `src/backend/bir/lir_to_bir/globals.cpp`, and
  `src/backend/bir/lir_to_bir/module.cpp` already distinguish raw/no-id
  initializer parsing from structured function initializer ids, including
  missing-id fail-closed behavior. Pointer initializer offsets are keyed by
  byte offset, and their `GlobalAddress.link_name_id` is filled from the
  resolved global/function target.
- `src/backend/prealloc/stack_layout/coordinator.cpp` already has a
  downstream prepared-address boundary that accepts `global_name_id` /
  `base_link_name_id` as authority and rejects mismatched raw spelling. That
  is a consumer proof target, not the first BIR lowering authority target for
  this packet.

Selected route:
- Harden dynamic global scalar-array materialization, specifically
  `DynamicGlobalScalarArrayAccess` in
  `src/backend/bir/lir_to_bir/memory/value_materialization.cpp`.
- First implementation target:
  carry a `LinkNameId` on `DynamicGlobalScalarArrayAccess`, populate it at the
  global GEP/addressing site that creates the access, and make
  `load_dynamic_global_scalar_array_value(...)` use that id directly instead
  of re-deriving semantic identity through `access.global_name`.
- Expected `LinkNameId` source:
  `GlobalInfo::link_name_id` from `global_types` when the dynamic global
  access is recognized from a metadata-rich LIR global.
- Fallback boundary:
  keep raw/no-id compatibility only when the selected access has
  `kInvalidLinkName`; if a structured id is present but missing from the module
  name table or mismatches the final spelling, fail closed rather than falling
  back through `global_types.find(access.global_name)`.
- Focused test location:
  extend `tests/backend/backend_lir_to_bir_notes_test.cpp`, near
  `expect_dynamic_global_scalar_array_loads_carry_link_name_id()`, with
  matching-id success plus stale/missing-id cases for dynamic global
  scalar-array loads.

## Suggested Next

Execute Step 2 for the selected route: add a `LinkNameId` carrier to
`DynamicGlobalScalarArrayAccess`, populate it from `GlobalInfo::link_name_id`
when constructing dynamic global scalar-array accesses, and update
`load_dynamic_global_scalar_array_value(...)` to use the carried id as the
semantic authority.

## Watchouts

- Do not re-prove only the addressed-global pointer route already closed by
  idea 187.
- Do not treat local route names, local slots, SSA temporaries, or block labels
  as semantic global symbols.
- Matching final spelling must not override mismatched `LinkNameId` metadata
  on the selected route.
- The dynamic global scalar-array route already has a positive drifted-display
  test. The missing coverage is fail-closed behavior when structured global
  identity is stale or unavailable on a metadata-rich path.
- Raw/no-id fallback should remain available only when the selected access has
  no structured id; preserving final BIR spelling is not the same as using raw
  spelling as semantic identity.

## Proof

Inventory-only packet; no build or test command required. No proof logs were
created or modified.
