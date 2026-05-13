Status: Active
Source Idea Path: ideas/open/210_aarch64_memory_operand_model_from_prepared_facts.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Document Memory Operand Contract And Close Readiness

# Current Packet

## Just Finished

Completed Step 5 from `plan.md`: proved volatility and address-space
preservation across the AArch64 target memory operand model.

Concrete work completed:
- Added direct memory operand proof that non-volatile `Gs` and volatile `Tls`
  records remain distinct and preserve non-default address spaces without
  target-local defaults.
- Extended prepared memory conversion proof so non-default `Gs`/`Tls` address
  spaces and volatile/non-volatile facts are preserved on successful
  frame-slot, global-symbol, pointer-value, and string-constant conversions.
- Added guard coverage for BIR/prepared volatility mismatch and for missing
  structured BIR address facts when prepared facts require size,
  non-default address space, or volatile behavior.
- Kept the slice record-only; no load/store selection, emission, object,
  call, or return behavior was added.

## Suggested Next

Execute Step 6 from `plan.md`: document the memory record contract and close
readiness, including supported/deferred surfaces and proof boundaries.

## Watchouts

- Keep this plan record-only; do not add load/store selection, assembly,
  encoding, object output, memory emission, calls, or returns.
- Preserve volatility and address-space facts from prepared input; do not
  invent target-local defaults.
- Step 6 should keep documentation scoped to record contracts and readiness;
  do not add wrappers, selection, assembly, encoding, object output, calls, or
  returns.
- The current conversion intentionally fails closed when prepared facts depend
  on structured BIR address facts that are absent or mismatched.
- String conversion preserves `TextId` only when the prepared text table has a
  matching symbol spelling; `LinkNameId` remains the structured identity
  carrier.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`

Result: backend subset passed with
`backend_aarch64_memory_operand_records` and
`backend_aarch64_prepared_memory_operand_records` volatility/address-space
coverage included and green: 130 tests passed, 0 failed; 12 disabled MIR trace
tests were not run. Proof log path: `test_after.log`.
