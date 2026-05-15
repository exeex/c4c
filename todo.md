Status: Active
Source Idea Path: ideas/open/241_f128_full_width_constant_carriers.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define The Full-Width Constant Carrier

# Current Packet

## Just Finished

Step 2 full-width constant carrier data model is complete. BIR `Value` now has
an exact F128 payload with explicit low/high 64-bit halves, a factory for
structured binary128 immediates, BIR/prepared printer rendering for structured
F128 literals, and prepared F128 carrier state that can publish an exact
constant payload without reusing F64, `double`, rendered text, or a single
64-bit lane. `backend_prepared_printer` proves the BIR literal, prepared
source literal, and shared F128 carrier dump preserve both halves.

## Suggested Next

Delegate Step 3 to transport real F128 constants through prepared value homes
or storage plans so normal backend selection can find the full-width payload by
prepared value identity instead of only in manually seeded carrier/printer
fixtures.

## Watchouts

- The Step 2 fixture seeds prepared F128 constant carrier state directly; it
  does not yet prove regalloc/storage-plan transport for lowered scalar F128
  constants.
- `immediate` and `immediate_bits` retain low-lane compatibility for existing
  scalar code, but F128 authority must come from `f128_payload`.
- Do not add AArch64 constant assembly printing until Step 3/4 can prove
  selected values have a complete structured payload.

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepare_liveness|backend_prepared_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_alu_records|backend_aarch64_prepared_scalar_cast_records|backend_aarch64_scalar_cast_records)$'; } 2>&1 | tee test_after.log`

Result: passed. Proof log: `test_after.log`.
