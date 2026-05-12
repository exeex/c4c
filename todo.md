Status: Active
Source Idea Path: ideas/open/187_bir_memory_provenance_global_handle_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory memory/provenance global-handle surfaces

# Current Packet

## Just Finished

Step 1 inventory completed for memory/provenance global-handle surfaces.

Classification:

- Route-local SSA/slot state:
  `ValueMap`, `LocalSlotTypes`, `LocalPointerSlots`,
  `LocalIndirectPointerSlotSet`, `LocalAddressSlots`,
  `LocalSlotAddressSlots`, `LocalSlotPointerValues`, `LocalArraySlotMap`,
  `DynamicLocalPointerArrayMap`, `DynamicLocalAggregateArrayMap`,
  `DynamicPointerValueArrayMap`, `LocalPointerArrayBaseMap`,
  `LocalAggregateSlotMap`, `LocalAggregateFieldSet`,
  `LocalPointerValueAliasMap`, `PointerAddressMap`, and
  `PointerAddressIntMap`. These are keyed by function-local SSA names, slot
  names, scratch names, or byte offsets inside local aggregate layout, and are
  not module global identity.
- Global symbol identity/provenance:
  `GlobalAddress`, `GlobalTypes`, `GlobalPointerMap`,
  `GlobalObjectPointerMap`, `GlobalAddressIntMap`,
  `GlobalObjectAddressIntMap`, `GlobalAddressSlots`,
  `AddressedGlobalPointerSlots`, `GlobalPointerValueSlots`,
  `AddressedGlobalPointerValueSlots`, `DynamicGlobalPointerArrayMap`,
  `DynamicGlobalAggregateArrayMap`, and `DynamicGlobalScalarArrayMap`.
  These carry or recover global provenance through direct global load/store,
  global GEP, ptrtoint/inttoptr, pointer-initializer, and dynamic global array
  paths.
- Display/diagnostics or layout text:
  `GlobalAddress::global_name`, dynamic global access `global_name` fields,
  `GlobalInfo::type_text`, `initializer_symbol_name`, local aggregate
  `type_text`/`storage_type_text`, and `GlobalPointerSlotKey::global_name`
  currently bridge to raw LIR operands, final BIR spelling, or compatibility
  aggregate layout text. They should not become independent semantic authority
  when a `LinkNameId` exists.
- Raw/no-id compatibility:
  `FunctionSymbolSet::raw_symbol_link_name_ids`,
  `GlobalTypes`/`TypeDeclMap` raw-name keys, direct `@global` LIR operand
  parsing, and textual pointer initializer import paths remain compatibility
  bridges for generated input that has not supplied a structured id. They must
  stay distinguishable from metadata-rich generated paths.

Selected Step 2 hardening target: `AddressedGlobalPointerSlots` and
`AddressedGlobalPointerValueSlots` keying through
`make_global_pointer_slot_key(const GlobalAddress&)`. The key currently uses
only `{global_name, byte_offset}` even when the source `GlobalAddress` carries
`link_name_id`; hardening should make generated global provenance keyed or
validated by `LinkNameId` and fail closed on stale/mismatched ids while
preserving raw/no-id compatibility.

## Suggested Next

Execute Step 2 from `plan.md`: harden
`AddressedGlobalPointerSlots`/`AddressedGlobalPointerValueSlots` by extending
`GlobalPointerSlotKey` or its validation path to include `LinkNameId` authority
for generated global provenance, then update load/store lookups that call
`make_global_pointer_slot_key`.

## Watchouts

- Retain compatibility for `kInvalidLinkName` paths: raw function-symbol import
  through `FunctionSymbolSet`, direct textual global operands before structured
  metadata exists, and legacy aggregate/type spelling used only for layout.
- Do not harden route-local maps such as local slots, SSA aliases, pointer
  value addresses, or dynamic local arrays; their string keys are intentional
  function-local handles.
- Direct global BIR operations already emit `global_name_id` through
  `link_name_id_for_global`; Step 2 should focus on provenance side-table
  identity, not final dump spelling.

## Proof

Inventory-only packet. No build/test proof was required or run, and no proof
logs were created or modified.
