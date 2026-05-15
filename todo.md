Status: Active
Source Idea Path: ideas/open/236_aarch64_i128_pair_lowering.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Print I128 Runtime Helper Boundary Calls

# Current Packet

## Just Finished

Step 7 prints supported direct-result i128 div/rem helper-boundary calls from
structured selected records and prepared helper marshaling facts.

What changed:

- `machine_printer.cpp` validates direct-result div/rem helper-boundary records
  for helper provenance, callee/result ownership, resource/clobber policy, ABI
  policy, live-preservation, and selected-call ownership before printing.
- Source marshal moves and result unmarshal moves are emitted from
  `PreparedI128RuntimeHelper::MarshalingMove` fields, not recovered from
  rendered names, source opcodes, helper callee strings, or fixed register
  conventions.
- The printable sequence is source lane moves to ABI argument registers,
  `bl <callee>`, then ABI result-register moves back to result carrier lanes.
- Incomplete live-preservation, selected-call ownership, missing marshal moves,
  or invalid ABI register bindings remain fail-closed with diagnostics.
- Marshal/unmarshal register names must be printable AArch64 `xN` GPR names;
  non-AArch64 helper ABI spellings fail closed instead of printing invalid
  assembly.
- Focused machine-printer tests cover signed div and unsigned rem direct-result
  helper output plus incomplete live-preservation, marshaling, and ABI facts.

## Suggested Next

Execute Step 8 validation and summarize the completed i128 pair-lowering route.
The validation packet should keep float/i128 conversion helpers and
memory-return helper families deferred unless the plan owner opens a separate
route for them.

## Watchouts

- The printer now emits direct-result div/rem helper calls only after selected
  call ownership is complete; keep incomplete live-preservation states
  fail-closed.
- Do not extend this printer path to float/i128 conversions or memory-return
  helpers without new prepared/selected ownership facts for those families.
- Keep float/i128 conversion helpers and memory-return helper families
  deferred.
- Future helper work must continue consuming marshal/unmarshal and ABI binding
  fields instead of adding fixed-register shortcuts.

## Proof

Passed:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepared_printer|backend_aarch64_target_instruction_records|backend_aarch64_target_record_core_contract|backend_aarch64_mir_carrier|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer)$') > test_after.log 2>&1
```

Result: build succeeded; `backend_prepare_liveness`,
`backend_prepared_printer`, `backend_aarch64_target_instruction_records`,
`backend_aarch64_target_record_core_contract`, `backend_aarch64_mir_carrier`,
`backend_aarch64_instruction_dispatch`, and
`backend_aarch64_machine_printer` passed, 7/7 tests. Proof log:
`test_after.log`.

Additional hygiene: `git diff --check` passed.

Supervisor full-suite acceptance: `(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1`
passed with regression guard against `test_before.log` copied from
`test_baseline.log`; before 3167/3167 and after 3167/3167.
