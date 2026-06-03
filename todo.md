Status: Active
Source Idea Path: ideas/open/104_bir_prealloc_pointer_carrier_provenance_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement The Carrier Provenance Contract

# Current Packet

## Just Finished

Step 3 implementation completed narrowly in the pointer-carrier publication
surface.

Implementation details:

- Added `PreparedPointerCarrierAuthority` to `PreparedPointerCarrierState` with
  explicit semantic authorities for `PreparedPointerValueAccess` and
  `BirPointerSymbol`.
- Added `has_semantic_pointer_carrier_authority` helpers so publication sites
  can reject carrier state that is not backed by an allowed authority.
- Renamed and isolated prepared pointer-value access seeding behind
  `update_prepared_pointer_value_access_step` and
  `prepared_pointer_value_access_carrier`.
- Isolated structured BIR link-name seeding behind
  `bir_pointer_symbol_carrier`, fed only by `pointer_symbol_link_name_id`
  validation/import through `prepared_pointer_symbol_name`.
- Kept no-address local-slot propagation only for already-authorized carriers.
  The slot map rejects invalid, zero-step, or non-semantic carrier state and
  preserves the existing carrier payload rather than deriving a new one.
- Removed the unresolved same-slot `+step_bytes` local-store route that minted
  successor carriers from local load/store adjacency.
- Removed addressed local/global store `-step_bytes` predecessor inference from
  recent load/store order. This packet did not add a new cross-layer explicit
  carrier relation, so those routes now fail closed for publication.
- Gated `classify_prepared_value_home` so `PointerBasePlusOffset` is published
  only from carriers with semantic authority.

Known follow-up proof needs for Step 4:

- Prove a raw local-slot load/store adjacency no longer mints a
  `PointerBasePlusOffset` home.
- Prove a no-address local slot can still preserve an already-authorized
  symbol or prepared pointer-value carrier unchanged.
- Prove pointer-symbol seeding uses valid structured link-name metadata and
  missing/invalid link-name metadata fails closed.
- Prove addressed local/global store plus/minus predecessor inference no longer
  publishes base+delta carrier facts without an explicit carrier relation.

## Suggested Next

Proceed to Step 4 and add focused tests for the three contract routes:
local-slot preservation/fail-closed behavior, pointer-symbol authority, and
plus/minus instruction-order fail-closed behavior.

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
- This packet intentionally did not add tests per the delegated ownership. Step
  4 should expect behavior changes around any previous tests that relied on
  order-derived `+step_bytes` or `-step_bytes` carriers.

## Proof

Passed:
`cmake --build --preset default 2>&1 | tee test_after.log; test ${PIPESTATUS[0]} -eq 0`

Proof log: `test_after.log`

Result:
Default build passed.
