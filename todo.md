Status: Active
Source Idea Path: ideas/open/207_aarch64_target_register_and_instruction_record_core.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add The Typed Register Vocabulary

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by adding the typed AArch64 register vocabulary in
`src/backend/mir/aarch64/abi/abi.hpp` and `.cpp`.

Work completed:
- Added typed `RegisterReference` values with explicit GP, stack-pointer, and
  FP/SIMD banks plus `x`, `w`, `sp`, `s`, `d`, `q`, and `v` views.
- Added constructors and validators for GP `x0`-`x30` / `w0`-`w30`, `sp`, and
  FP/SIMD `s0`-`s31`, `d0`-`d31`, `q0`-`q31`, and `v0`-`v31`.
- Added role helpers and predicates for frame pointer, link register, stack
  pointer, sret, indirect-call scratch, platform-reserved, caller-saved, and
  callee-saved AAPCS64 groups.
- Added `backend_aarch64_register_vocabulary` coverage for representative GP,
  SP, FP/SIMD, special-role, caller-saved, and callee-saved classification.

## Suggested Next

Step 3 packet: add prepared-register conversion helpers in the AArch64 ABI
owner that parse/validate prepared physical-register strings, banks, classes,
and views into the typed `RegisterReference` vocabulary, failing closed on
unknown names or mismatches.

Recommended proof can stay focused on the new conversion test plus the existing
backend subset selected by the supervisor.

## Watchouts

- Keep `module/` as a prepared/BIR snapshot boundary.
- Do not add instruction selection, assembly emission, object output, or linker
  behavior.
- Step 2 intentionally does not parse prepared physical-register strings.
  Step 3 should be the first point that treats prepared strings/banks/classes
  as candidates for typed AArch64 registers, and it should fail closed on
  unknown or mismatched facts.
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

Result: passed, including `backend_aarch64_register_vocabulary`.

Proof log: `test_after.log`.
