Status: Active
Source Idea Path: ideas/open/103_prealloc_synthetic_helper_call_abi_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Helper Proof

# Current Packet

## Just Finished

Step 4 focused proof is complete.

Coverage recorded:

- i128 helper ABI binding: `tests/backend/bir/backend_prepare_liveness_test.cpp` now asserts both div/rem helpers and supported float/integer conversion helpers satisfy `prepared_i128_runtime_helper_has_abi_contract`. Existing assertions cover callee identity, source opcode/cast, source/result types and widths, ABI transition, banks/counts/lanes, lane/scalar ownership, marshaling, clobber policy, and selected ownership.
- f128 arithmetic helper ABI binding: `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp` asserts the arithmetic helper consumed by AArch64 satisfies `prepared_f128_runtime_helper_has_abi_contract`, alongside existing helper kind/opcode/callee, ABI register, selected ownership, clobber, and fail-closed checks.
- f128 cast helper ABI binding: strengthened `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp` so both scalar-to-f128 (`__extenddftf2`) and f128-to-scalar (`__trunctfsf2`) helper records must satisfy `prepared_f128_runtime_helper_has_abi_contract`. Existing prepared-printer, dispatch, and machine-printer assertions cover the cast ABI transitions and physical emission.
- f128 comparison bridge: `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp` already asserts `prepared_f128_runtime_helper_has_scalar_cmp_result_bridge_contract` for the `__eqtf2` helper, including helper `I32` result, BIR `I1` result ownership, zero-test, `consumes_helper_cmp_result`, and `owns_bir_i1_result`. Existing dispatch and machine-printer tests prove AArch64 materializes the I1 result from that prepared bridge.

Only focused tests were strengthened. No implementation files were changed in this packet.

## Suggested Next

Proceed to Step 5 final validation and close-readiness notes. Run the delegated broader validation, then record final coverage and retained prepared-only authority details in `todo.md`.

## Watchouts

- The source tree already contains Step 3 implementation changes from the prior packet; this Step 4 packet intentionally touched only owned tests plus `todo.md` and `test_after.log`.
- The prepared-only model remains the accepted authority. Do not add source BIR direct-call modeling or move physical call planning out of prealloc/MIR during final validation.
- The prepared-printer still proves visible helper details without directly calling the new predicates, to avoid adding linker dependencies to printer-only tests.

## Proof

Passed:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepared_printer|backend_aarch64_target_instruction_records|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer)$') > test_after.log 2>&1`

`test_after.log`: build completed and 5/5 targeted backend tests passed.
