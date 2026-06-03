Status: Active
Source Idea Path: ideas/open/105_prealloc_raw_global_address_identity_fallback_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Raw Global Identity Fallback Sites

# Current Packet

## Just Finished

Step 1 inventory completed for raw global identity fallback sites in prepared
address and global materialization construction.

Prepared address schema:
- `src/backend/prealloc/addressing.hpp` carries global identity as
  `PreparedAddress::symbol_name` / `PreparedAddressMaterialization::symbol_name`
  (`LinkNameId` in prepared names). It does not preserve a separate raw spelling
  channel for global symbols. `prepared_global_symbol_address_policy` is a
  fail-closed consumer when a global-symbol address lacks `symbol_name`.
- Owner classification: prepared-address schema/prealloc consumer boundary.
  Contract gap: once raw spelling is interned as `symbol_name`, downstream code
  cannot distinguish structured identity from raw compatibility.

Raw fallback sites found:
- `build_direct_symbol_backed_address` no-address route for ordinary
  `LoadGlobalInst` / `StoreGlobalInst`: `resolve_symbol_name` uses
  `global_name_id` when present, but falls back to nonempty `global_name` when
  the ID is `kInvalidLinkName`; `find_global_by_symbol` also resolves policy by
  raw `Global::name` or raw spelling of a declared link name when no ID exists.
  Current behavior: raw/no-id ordinary global load/store can become a
  `PreparedAddressBaseKind::GlobalSymbol` with a prepared `symbol_name` and
  policy. Classification: compatibility fallback that currently becomes
  semantic-looking identity authority. Likely owner: prealloc stack-layout
  coordinator, with BIR lowering/validation responsible for publishing
  structured `global_name_id` for normal lowered globals.
- `build_direct_symbol_backed_address` explicit `MemoryAddress::GlobalSymbol`
  route for addressed loads/stores: `base_link_name_id` is authoritative when
  present, but raw `MemoryAddress::base_name` is accepted when the link-name ID
  is missing. When the explicit address lacks `base_name`, the helper may use
  the instruction fallback global ID/name; when `base_name` is present without
  an ID it is treated as raw compatibility. Current behavior: global
  GEP-derived/addressed global routes can publish a prepared symbol from raw
  spelling. Classification: compatibility fallback that becomes
  semantic-looking identity authority. Likely owner: BIR memory-address
  producers for `base_link_name_id`, prealloc coordinator for fail-closed
  consumption.
- `find_global_by_symbol` policy lookup in the same helper: if `LinkNameId` is
  missing it searches module globals by `global.name` and by the BIR link-name
  spelling. Current behavior: raw spelling can select the target global used for
  address-materialization policy/TLS eligibility. Classification:
  compatibility fallback with ordinary identity-authority consequences. Likely
  owner: prealloc stack-layout coordinator; target relocation/TLS policy itself
  remains out of scope.
- `resolve_prepared_text_id` and `direct_string_constant_name` accept raw string
  names, including pointer values named `@...` for string constants. Current
  behavior: display/raw compatibility for string constants, not global symbol
  identity. Classification: display-only/string compatibility fallback. Likely
  owner: prealloc string-constant materialization; source idea treats this as
  adjacent unless it reuses the global-symbol contract.
- `append_direct_global_address_materialization` for pointer results and pointer
  values requires `Value::pointer_symbol_link_name_id`; missing ID records a
  note for `@...` values and returns without materializing a global symbol.
  Current behavior: no raw fallback for direct/GOT/TLS global materialization.
  Classification: fail-closed input with structured BIR pointer-symbol metadata
  as authority. Likely owner: BIR value metadata producers plus prealloc
  materialization consumer.
- `append_pointer_value_address_materialization` delegates to direct global
  materialization only when `pointer_symbol_link_name_id` is present. Without an
  ID it may materialize frame slots or string constants, but not globals.
  Current behavior: global pointer-value materialization is structured/fail
  closed; string constants retain raw display compatibility. Classification:
  fail-closed for global identity, display-only fallback for strings. Likely
  owner: prealloc address-materialization publication.
- Global pointer initializer route: BIR global initializers carry
  `initializer_symbol_name_id` and initializer elements carry
  `Value::pointer_symbol_link_name_id`; prealloc materialization consumes the
  value metadata path above. Current behavior in prealloc is structured/fail
  closed for global materialization, while BIR lowering/validation retains
  raw/no-id compatibility for some initializer imports. Classification:
  semantic identity belongs to BIR; prealloc should not mint missing initializer
  IDs from raw spelling.
- Adjacent non-addressing sites: `control_flow.hpp`
  `resolve_prepared_bir_link_name_ref` and `bir_link_name_or_raw`, plus
  `call_plans.cpp` `resolve_symbol_pointer_name`, also accept raw spelling when
  structured IDs are missing. These are not the primary prepared address /
  materialization constructors, but they are live compatibility fallbacks that
  can affect prepared same-module global references or symbol-address storage
  display. Classification: compatibility fallback outside the Step 1 primary
  target. Likely owner: control-flow/call-plan prepared contracts.

Existing proof surfaces:
- `backend_lir_to_bir_notes` already proves structured `LinkNameId` survives
  drifted displays, ordinary global load/store mismatches are rejected by BIR
  validation, raw/no-id ordinary global compatibility is still accepted, and
  pointer initializer symbol names carry IDs where available.
- `backend_aarch64_prepared_memory_operand_records` proves prepared consumers
  reject global-symbol mismatches/missing prepared symbol facts and consume
  structured `base_link_name_id` for addressed global operands.
- `backend_x86_handoff_boundary_i32_guard_chain` contains drifted raw global
  carrier tests proving prepared same-module global accesses are authoritative
  over raw fallback after prepared publication, plus rejection after prepared
  access loss.

Candidate contract gaps for Step 2:
- Decide whether raw/no-id ordinary global load/store prepared-address
  construction remains a named compatibility fallback or fails closed.
- Decide whether explicit `MemoryAddress::GlobalSymbol` without
  `base_link_name_id` may publish `PreparedAddress::symbol_name`, especially
  for GEP-derived addresses.
- Decide whether `find_global_by_symbol` can use raw spelling for policy lookup
  once prepared global identity is expected to be structured.
- Keep direct/GOT/TLS global materialization fail-closed on
  `pointer_symbol_link_name_id`; do not introduce raw materialization fallback.

## Suggested Next

Execute `plan.md` Step 2: choose the narrow raw-global identity authority
contract for ordinary global accesses, explicit/GEP-derived
`MemoryAddress::GlobalSymbol`, raw/no-id compatibility, and fail-closed direct
global materialization.

## Watchouts

- Do not make raw symbol spelling the primary identity path when structured
  `LinkNameId` exists.
- Keep final target relocation selection and TLS lowering out of scope.
- Separate display-only compatibility from semantic global identity authority.
- Watch the schema limitation: prepared global addresses currently have only
  `symbol_name`, so retaining raw compatibility may need an explicit marker or
  named helper boundary if downstream code must distinguish it.
- Existing direct global materialization already fails closed on missing
  `pointer_symbol_link_name_id`; Step 2 should avoid weakening that path.
- Adjacent control-flow/call-plan raw fallbacks exist but are outside the
  primary prepared-address/materialization construction target unless Step 2
  intentionally broadens ownership.

## Proof

Passed. Ran:
`git diff --quiet -- src/backend/bir src/backend/prealloc tests && printf 'analysis-only proof: no implementation or test diff for raw global identity fallback inventory\n' > test_after.log`

Proof log: `test_after.log`.
