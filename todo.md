Status: Active
Source Idea Path: ideas/open/104_bir_prealloc_pointer_carrier_provenance_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Provenance Proof

# Current Packet

## Just Finished

Step 4 focused provenance proof added in
`tests/backend/bir/backend_prepare_stack_layout_test.cpp`.

Test coverage added:

- Added `check_pointer_carrier_contract_value_homes`, a direct prepared
  value-home contract test built from a small BIR function, prepared addressing
  record, and synthetic regalloc value list.
- Local-slot preservation: a valid `pointer_symbol_link_name_id` carrier stored
  through a no-address local slot is preserved unchanged into
  `contract.symbol.copy.ptr` and publishes a symbol-backed
  `PointerBasePlusOffset` home.
- Local-slot prepared-pointer preservation: a carrier seeded from
  `PreparedAddressBaseKind::PointerValue` survives no-address local slot
  storage unchanged into `contract.prepared.copy.ptr`.
- Local-slot fail-closed: raw same-slot load/store adjacency for
  `contract.adjacent.advanced.ptr` does not mint a new
  `PointerBasePlusOffset` home from slot spelling or instruction order.
- Local-slot overwrite guard: the unauthorized adjacent store does not replace
  the existing authorized slot carrier; `contract.adjacent.after.ptr` still
  resolves to the original prepared pointer-value carrier.
- Pointer-symbol fail-closed: missing/invalid pointer-symbol link-name metadata
  for `contract.missing.copy.ptr` does not publish symbol or base/delta carrier
  facts.
- Plus/minus fail-closed: addressed local and global store predecessor
  candidates, `contract.local.previous.ptr` and
  `contract.global.previous.ptr`, do not publish base+delta carrier facts
  without an explicit carrier relation.
- Follow-up review fix: the plus/minus fail-closed fixture now includes
  symbol-backed recent pointer loads immediately before both addressed
  predecessor stores. This covers the old order-derived route precondition
  where `last_loaded_pointer_state` existed before storing an unresolved
  pointer value through `MemoryAddress::PointerValue`; the old `-step_bytes`
  inference would have published base+delta homes for these candidates.

## Suggested Next

Proceed to Step 5 final validation and close-readiness notes after supervisor
review of whether broader backend validation is needed for the Step 3/4 slice.

## Watchouts

- Do not treat load/store instruction shape as target-neutral provenance
  authority without a structured fact.
- Keep register allocation, storage homes, spill decisions, and target
  placement in prealloc.
- Separate transient regalloc carrier metadata from durable BIR/prepared
  pointer provenance.
- Avoid adding slot-name parsing, testcase-shaped pointer pairing, or a contract
  that makes same-slot load/store order semantic authority.
- `base_symbol_name` currently only rides on carrier state; make sure Step 2
  distinguishes BIR symbol provenance from physical indirect-call or
  publication-plan movement.
- Step 3 should prefer fail-closed behavior over preserving the current
  plus/minus relation by renaming helpers.
- If preserving any carrier through local slots, prove the carrier is unchanged
  and already authorized before it reaches `classify_prepared_value_home`.
- The focused proof uses `regalloc_detail::build_prepared_value_homes` directly
  to stay at the carrier-publication boundary; it does not prove final target
  assembly symptoms.
- Existing source changes from Step 3 remain part of the active slice; this
  packet did not edit `src/backend/**`.

## Proof

Passed:
`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -R "backend_prepare_stack_layout|backend_lir_to_bir_notes|backend_prepared_lookup_helper|backend_prealloc_decoded_home_storage" --output-on-failure; } 2>&1 | tee test_after.log`

Proof log: `test_after.log`

Result:
Build passed and focused CTest subset passed: `backend_lir_to_bir_notes`,
`backend_prepare_stack_layout`, `backend_prepared_lookup_helper`, and
`backend_prealloc_decoded_home_storage`. The follow-up proof used the same
command after adding recent-load preconditions for the plus/minus fail-closed
cases.
