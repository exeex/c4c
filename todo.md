Status: Active
Source Idea Path: ideas/open/104_bir_prealloc_pointer_carrier_provenance_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Final Validation And Close Readiness

# Current Packet

## Just Finished

Step 5 final backend validation passed after the explicit computed-pointer
authority repair.

Source idea acceptance coverage:

- Each pointer-carrier provenance route is classified and implemented as either
  retained transient prealloc metadata or explicit semantic authority:
  prepared pointer-value access facts, BIR pointer-symbol link-name facts, and
  explicit BIR pointer add/sub immediate relations from an authorized base.
- Local-slot propagation no longer relies on raw slot spelling or nearby
  load/store order to mint semantic pointer provenance. It may preserve an
  already-authorized carrier unchanged through no-address slots.
- Pointer-symbol carrier seeding has structured authority through valid BIR
  `pointer_symbol_link_name_id` metadata; missing/invalid link-name metadata
  fails closed.
- Pointer plus/minus instruction-order derivation fails closed. The focused
  proof includes the old recent-load precondition and still rejects base+delta
  publication without an explicit carrier relation.
- Computed pointer call-argument source shape is retained only through an
  explicit BIR pointer-typed add/sub immediate relation from an already
  authorized base carrier, not through local-slot/order inference.

Focused proof coverage retained:

- Local-slot preservation: valid BIR symbol carriers and prepared pointer-value
  carriers preserve unchanged through no-address local slots.
- Local-slot fail-closed: raw same-slot load/store adjacency does not mint a new
  `PointerBasePlusOffset` home, and unauthorized raw stores do not replace the
  existing authorized slot carrier.
- Pointer-symbol seeding/fail-closed: valid structured link-name metadata
  publishes a symbol-backed carrier; missing/invalid link-name metadata fails
  closed.
- Plus/minus fail-closed: addressed local/global predecessor candidates include
  the old recent-load precondition and do not publish base+delta carrier facts
  without an explicit carrier relation.
- Computed-pointer authority: `backend_prepare_frame_stack_call_contract` and
  `backend_prepared_printer` cover explicit `loaded.ptr + 4` pointer source
  shape through call-plan and printer contracts.

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

- Close-ready for plan-owner closure review. The final delegated backend-wide
  validation passed, and the source idea acceptance criteria are covered
  without restoring raw slot/order-derived pointer provenance, weakening tests,
  or moving physical placement authority out of prealloc/MIR.

## Suggested Next

Hand off to the supervisor/plan-owner for closure review of
`ideas/open/104_bir_prealloc_pointer_carrier_provenance_contract.md`.

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
`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -R "^backend_" -j --output-on-failure; } 2>&1 | tee test_after.log`

Proof log: `test_after.log`

Result:
Build passed and backend CTest passed: 169/169 tests passed, 0 failed.
