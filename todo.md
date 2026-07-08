Status: Active
Source Idea Path: ideas/open/620_prepared_mixed_object_data_slots.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add explicit relocation-slot prepared facts

# Current Packet

## Just Finished

- Completed Step 2 schema/verifier support for explicit relocation-slot
  prepared facts.
- Added `PreparedObjectDataRelocationSlot` with byte offset, byte size, and
  target link identity, and added `relocation_slots` to
  `PreparedGlobalObjectData`.
- Taught selected object-data verifier facts to report missing relocation
  slots separately from missing relocation presence, and to reject targetless,
  zero-sized, out-of-range, duplicate, or overlapping slots as relocation
  incoherence.
- Preserved the accepted `608` relocation-only object-data progress by
  publishing a whole-object relocation slot for one-slot pointer object data.
- No mixed row is populated as coherent yet; producer population remains Step 3.

## Suggested Next

Supervisor should route Step 2 completion through plan-owner. The next
executable packet should be Step 3 producer population: convert BIR initializer
evidence into emitted bytes plus relocation slots only when object size,
alignment, slot offset, slot target, and ordinary byte ranges are known.

## Watchouts

- Do not route RV64 relocation-record emission, byte emission, symbol
  materialization, or access-width policy into this plan.
- Do not mark mixed aggregate object data coherent without prepared emitted
  byte spans plus relocation slot offsets and target identity.
- Preserve `608` parked evidence for prepared global memory facts and direct
  global-symbol base-plus-offset authority; do not repeat helper-only
  `ByteStorageAggregate` publication as progress.
- Preserve relocation-only object-data progress from `608`, including the
  `src/921110-1.c` move to the RV64 relocation-record consumer stop.
- Treat `src/20010924-1.c` as a representative, not a named-case shortcut.
- Do not populate mixed rows as coherent in Step 2; producer population belongs
  to Step 3.
- Step 2 should stop if it needs BIR initializer producer population or RV64
  relocation-record emission to prove movement.
- Keep Step 3 semantic across the mixed family; do not special-case
  `src/20010924-1.c`.

## Proof

- Build:
  `cmake --build --preset default --target backend_prealloc_prepared_contract_verifier_test backend_riscv_object_emission_test`
- Focused tests:
  `ctest --test-dir build -R '^backend_prealloc_prepared_contract_verifier$|^backend_riscv_object_emission$' --output-on-failure`
- Result: both focused tests passed.
