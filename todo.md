Status: Active
Source Idea Path: ideas/open/620_prepared_mixed_object_data_slots.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Populate mixed bytes and relocation slots

# Current Packet

## Just Finished

- Accepted Step 2 schema/verifier support for explicit relocation-slot
  prepared facts and advanced canonical execution to Step 3.
- Added `PreparedObjectDataRelocationSlot` with byte offset, byte size, and
  target link identity, and added `relocation_slots` to
  `PreparedGlobalObjectData`.
- Taught selected object-data verifier facts to report missing relocation
  slots separately from missing relocation presence, and to reject targetless,
  zero-sized, out-of-range, duplicate, or overlapping slots as relocation
  incoherence.
- Preserved the accepted `608` relocation-only object-data progress by
  publishing a whole-object relocation slot for one-slot pointer object data.
- No mixed row is populated as coherent yet; producer population is now the
  active Step 3 packet.

## Suggested Next

Next executable packet is Step 3 producer population. Capture the current
selected object-data diagnostics and prepared facts for `src/20010924-1.c`
plus at least two neighboring selected object-data rows, then edit only
`src/backend/prealloc/object_data.cpp` producer logic if the evidence shows
BIR initializer/global layout facts can supply object size, alignment, emitted
byte ranges, relocation slot offsets, and relocation target identity.

Recommended proof command:
`{ cmake --build --preset default && ALLOWLIST=build/agent_state/620_step3_mixed_object_data.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1`

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
- Step 3 may populate prepared facts only from BIR initializer/global layout
  evidence. It must not reconstruct relocation records, section bytes, or
  symbol materialization in RV64.
- Stop and return evidence if the first remaining blocker is RV64
  relocation-record emission, byte emission, symbol materialization, or
  access-width policy.

## Proof

- Build:
  `cmake --build --preset default --target backend_prealloc_prepared_contract_verifier_test backend_riscv_object_emission_test`
- Focused tests:
  `ctest --test-dir build -R '^backend_prealloc_prepared_contract_verifier$|^backend_riscv_object_emission$' --output-on-failure`
- Result: both focused tests passed.
