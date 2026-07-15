# Current Packet

Status: Step 3 complete
Source Idea Path: ideas/open/761_lir_call_signature_type_mirror_convergence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Address selector and inline-assembly boundaries

## Just Finished

- Step 3: made `LirSwitch::selector_type_ref` the structured selector type
  authority, populated it during HIR lowering, checked `selector_type` only as
  a display mirror, and rendered switch text from the structured type. Added
  registered backend interface coverage showing stale `i64` selector text
  cannot override structured `i32` facts and a structured type mismatch still
  fails.

## Suggested Next

- Select the next bounded runbook packet; retain the explicit inline-assembly
  boundary rather than widening this completed switch selector slice.

## Watchouts

- `selector_type_ref` is appended after the existing semantic selector field
  to preserve legacy aggregate-producer field ordering. Inline assembly was
  intentionally not changed by this packet.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log` — passed (5/5 backend tests, including `backend_lir_to_bir_interface`); proof log: `test_after.log`.
