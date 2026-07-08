Status: Active
Source Idea Path: ideas/open/620_prepared_mixed_object_data_slots.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Populate mixed bytes and relocation slots
你該做test baseline review了

# Current Packet

## Just Finished

- Completed Step 3 producer population for mixed bytes plus relocation slots.
- Added BIR `GlobalInitializerRelocationSlot` metadata and preserved aggregate
  pointer-initializer byte offsets during LIR-to-BIR lowering.
- Interned addressable string-pool relocation-slot targets with BIR-local
  link-name identity without changing ordinary string literal operand identity.
- Taught prepared object-data production to publish coherent mixed
  `emitted_bytes` plus `relocation_slots` only when object extent, alignment,
  slot offset, slot width, slot target, and ordinary byte ranges are known and
  non-overlapping.
- Added focused regression coverage for BIR aggregate relocation-slot
  publication and prepared mixed object-data facts.
- Proof moved `src/20010924-1.c` from prepared selected object-data contract
  `unsupported_but_coherent` to the later RV64 relocation-record diagnostic:
  `RV64 object route cannot emit prepared relocation object data without
  relocation records`.
- Neighboring rows `src/pr61517.c`, `src/pr57877.c`, `src/pr57860.c`, and
  `src/20030224-2.c` remained fail-closed at the prepared selected
  object-data contract stop with `unsupported_but_coherent`.

## Suggested Next

Next executable packet is Step 4 handoff proof. Record the Step 3 row movement,
confirm the later RV64 relocation-record diagnostic belongs to
`ideas/open/609_rv64_global_data_consumer.md`, and return to lifecycle routing
after the code slice is accepted.

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
  `cmake --build --preset default --target backend_lir_to_bir_notes_test backend_prealloc_prepared_contract_verifier_test backend_riscv_object_emission_test`
- Focused tests:
  `ctest --test-dir build -R '^backend_lir_to_bir_notes$|^backend_prealloc_prepared_contract_verifier$|^backend_riscv_object_emission$' --output-on-failure`
- Narrow RV64 allowlist:
  `{ cmake --build --preset default && ALLOWLIST=build/agent_state/620_step3_mixed_object_data.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1`
- Result: focused tests passed; allowlist ran 5 rows, passed 0, failed 5.
  `src/20010924-1.c` moved to the RV64 relocation-record diagnostic; the
  other four rows retained the prepared object-data contract stop.
