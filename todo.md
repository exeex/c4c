Status: Active
Source Idea Path: ideas/open/207_aarch64_target_register_and_instruction_record_core.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Prepared Register Conversion Helpers

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by adding prepared-register conversion helpers in
`src/backend/mir/aarch64/abi/abi.hpp` and `.cpp`.

Work completed:
- Added `parse_aarch64_register_name` and `convert_prepared_register` helpers
  that validate prepared physical-register spellings into typed
  `RegisterReference` values.
- Added explicit conversion failure kinds for empty/unknown names, unsupported
  views, bank mismatches, class mismatches, and expected-view mismatches.
- Added conversion overloads for `PreparedPhysicalRegisterAssignment` and
  `PreparedSavedRegister` carriers.
- Added `backend_aarch64_prepared_register_conversion` coverage for
  representative GP, SP, FP/SIMD successes plus unknown, unsupported-view,
  bank-mismatch, class-mismatch, and expected-view mismatch failures.

## Suggested Next

Step 4 packet: add target operand record structs under the AArch64 target-record
owner, using the typed register vocabulary and prepared-register conversion
helpers as the register boundary.

## Watchouts

- Keep `module/` as a prepared/BIR snapshot boundary.
- Do not add instruction selection, assembly emission, object output, or linker
  behavior.
- Prepared register conversion accepts aggregate-address class/bank metadata for
  GP physical registers because those prepared facts still name AArch64 address
  registers.
- The conversion layer rejects uppercase aliases, zero-padded names, `x31`, and
  prepared `sp` with GP/FPR/VREG bank metadata.
- `x18` is exposed as platform-reserved and is excluded from caller/callee
  saved groups.
- Avoid extending `src/backend/mir/aarch64/codegen/emit.hpp`; it still exposes
  raw BIR/LIR text-era surfaces and is not the target-record owner for this
  plan.

## Proof

Ran:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log
```

Result: passed, including `backend_aarch64_register_vocabulary` and
`backend_aarch64_prepared_register_conversion`.

Proof log: `test_after.log`.
