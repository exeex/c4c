Status: Active
Source Idea Path: ideas/open/242_aarch64_barrier_cache_hint_builtin_intrinsic_carriers.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Prepared Intrinsic Carriers

# Current Packet

## Just Finished

Plan Step 3 - Publish Prepared Intrinsic Carriers completed for the
no-result AArch64 DMB barrier representative.

Implemented prepared carrier authority:

- Extended `PreparedIntrinsicCarrier` with structured barrier-domain and
  immediate-value fields.
- Added a barrier validator that accepts only complete `BarrierDmb` facts with
  void result, side effects, `sy` domain, required immediate value `15`, one
  domain operand, no memory access, and no target feature requirement.
- Printed the prepared barrier-domain and immediate facts without adding any
  selected-machine spelling.
- Proved malformed domain, non-immediate domain, and missing-side-effect
  barrier facts remain missing, while scalar/CRC/vector carriers still pass.
- Added an AArch64 dispatch boundary check showing a complete DMB carrier still
  reports unsupported intrinsic family instead of selecting an instruction or
  falling back to a call.

## Suggested Next

Proceed to `plan.md` Step 4 for the same barrier representative: keep AArch64
machine dispatch/printer boundaries closed for complete barrier carriers and
record that no selected DMB machine-node support exists in this plan slice.

## Watchouts

- The complete prepared DMB carrier intentionally has no result home and no
  value home for the immediate operand; the immediate value itself is the
  carrier authority.
- Barrier selected-machine support is still absent by design. Do not add DMB
  records, machine-printer spellings, or call/atomic-fence substitutions in
  Step 4.
- Cache-maintenance, pause/hint, and builtin-address representatives remain
  outside this completed barrier packet.

## Proof

Delegated proof for the Step 3 prepared intrinsic carrier packet:

- `set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(lir_to_bir_notes|prepared_printer|aarch64_instruction_dispatch)'; } 2>&1 | tee test_after.log`
- Result: passed, 3/3 tests green.
- Proof log: `test_after.log`
