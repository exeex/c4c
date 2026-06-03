Status: Active
Source Idea Path: ideas/open/104_bir_prealloc_pointer_carrier_provenance_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Explicit Computed Pointer Carrier Authority

# Current Packet

## Just Finished

Step 5 backend validation blocker repaired with explicit computed-pointer
authority.

Implementation details:

- Added `PreparedPointerCarrierAuthority::BirPointerImmediateOffset` as a
  semantic carrier authority for explicit pointer-typed BIR `BinaryInst`
  add/sub immediate relations.
- `pointer_carriers.cpp` now publishes a computed pointer carrier only when the
  BIR instruction itself names the relation, such as `derived = base + 4`, and
  the pointer base already resolves to an authorized carrier.
- The computed carrier records the BIR binary operand as
  `base_value_name`, preserves any structured symbol name from the authorized
  base carrier, carries the explicit immediate delta, and uses the base
  carrier step size.
- The removed raw same-slot `+step_bytes` route and addressed-store
  `-step_bytes` recent-load route were not restored.
- Updated the two failing source-shape fixtures so `derived.seed` is explicitly
  produced by `BinaryInst{Ptr Add loaded.ptr, 4}` before being stored and
  reloaded for the call argument. The fixtures no longer depend on a phantom
  local-slot/order relation.

Coverage already established before this blocker:

- Local-slot preservation: valid BIR `pointer_symbol_link_name_id` carriers and
  prepared pointer-value carriers preserve unchanged through no-address local
  slots in the focused value-home test.
- Local-slot fail-closed: raw same-slot load/store adjacency does not mint a new
  `PointerBasePlusOffset` home, and unauthorized raw stores do not replace the
  existing authorized slot carrier.
- Pointer-symbol seeding: valid structured link-name metadata publishes a
  symbol-backed carrier; missing/invalid link-name metadata fails closed.
- Plus/minus fail-closed: addressed local/global predecessor candidates include
  the old recent-load precondition and do not publish base+delta carrier facts
  without an explicit carrier relation.

Retained transient metadata details:

- `slot_pointer_carriers` may preserve an already-authorized carrier unchanged
  through no-address local slots.
- Prepared pointer-value access self carriers are projections of
  `PreparedAddressBaseKind::PointerValue` access facts.
- Symbol self carriers are projections of BIR `pointer_symbol_link_name_id`
  metadata.
- Register homes, stack homes, call-argument moves, and target placement remain
  prealloc/MIR physical facts and are not provenance authority.

Close-readiness status:

- Focused blocker repair is ready for supervisor review. The previously failing
  call-plan and prepared-printer source-shape tests pass with explicit computed
  pointer authority, and the local-slot/symbol/plus-minus focused carrier
  contract tests still pass.
- A final `^backend_` validation pass is still needed before lifecycle close
  readiness because the last broad Step 5 run failed before this repair.

## Suggested Next

Rerun Step 5 final backend validation with `ctest -R "^backend_"` and record
close-readiness if the broader guard is green.

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
- Do not satisfy the blocker by weakening
  `backend_prepare_frame_stack_call_contract` or `backend_prepared_printer`.
- Do not reintroduce generic `+step_bytes` / `-step_bytes` derivation from
  recent load/store order as semantic pointer provenance.
- If later tests need computed pointer source shape, require an explicit BIR
  pointer add/sub immediate relation from an already-authorized base carrier.

## Proof

Passed:
`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -R "backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_prepare_stack_layout|backend_lir_to_bir_notes|backend_prepared_lookup_helper|backend_prealloc_decoded_home_storage" --output-on-failure; } 2>&1 | tee test_after.log`

Proof log: `test_after.log`

Result:
Build passed and focused CTest subset passed: `backend_lir_to_bir_notes`,
`backend_prepare_stack_layout`, `backend_prepare_frame_stack_call_contract`,
`backend_prepared_printer`, `backend_prepared_lookup_helper`, and
`backend_prealloc_decoded_home_storage`.
