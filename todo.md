Status: Active
Source Idea Path: ideas/open/103_prealloc_synthetic_helper_call_abi_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Document The Contract

# Current Packet

## Just Finished

Step 3 implementation is complete.

Implemented prepared-only contract helpers:

- Added `prepared_i128_runtime_helper_has_abi_contract` in `src/backend/prealloc/i128_runtime_helpers.hpp/.cpp`. It names the prepared-only i128 helper ABI contract for div/rem and supported float/integer conversion helpers, including helper family, source opcode/cast shape, source/result type and width facts, result ownership, ABI transition, argument/result banks, lane counts, and lane width.
- Added `prepared_f128_runtime_helper_has_abi_contract` in `src/backend/prealloc/f128_runtime_helpers.hpp/.cpp`. It names the prepared-only f128 helper ABI contract for arithmetic, comparison, and cast helpers, including helper family, source opcode/cast shape, source/result type facts, result ownership, ABI transition, argument/result banks/counts, and width.
- Added `prepared_f128_runtime_helper_has_scalar_cmp_result_bridge_contract` in `src/backend/prealloc/f128_runtime_helpers.hpp/.cpp`. This is the named f128 comparison bridge contract: helper result `I32`, BIR result `I1`, non-missing predicate zero-test, `consumes_helper_cmp_result=true`, `owns_bir_i1_result=true`, scalar result ownership, GPR ABI result binding, and `AbiCmpResultToScalar` unmarshal direction.

Consumer tightening:

- `src/backend/mir/aarch64/codegen/i128_ops.cpp` now consumes the prepared i128 ABI contract predicate for record construction, instruction selection, and printing checks instead of relying only on local ABI-policy shape checks. It also guards invalid prepared-helper provenance before dereferencing source-helper pointers.
- `src/backend/mir/aarch64/codegen/f128.cpp` now consumes the prepared f128 ABI contract predicate and the named f128 comparison bridge predicate before AArch64 record materialization. The consumer still owns physical materialization of the I1 result register after the prepared bridge contract is valid.
- No synthetic helper was modeled as a source BIR direct call, and no physical clobber, preservation, carrier movement, or register/stack placement authority was moved out of prealloc/MIR.

Focused proof added/strengthened:

- `tests/backend/bir/backend_prepare_liveness_test.cpp` asserts i128 helper mappings satisfy the named prepared ABI contract.
- `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp` asserts i128 and f128 helper boundary records consume helpers satisfying the named prepared ABI contract, and asserts the f128 comparison helper satisfies the named I32-to-I1 bridge contract.
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp` keeps the incomplete i128 helper fail-closed fixture valid under the stricter prepared-helper provenance check by using a valid prepared helper with intentionally incomplete live-preservation facts.

## Suggested Next

Proceed to Step 4 by reviewing proof coverage for scalar i128 helper ABI binding, f128 arithmetic/cast helper ABI binding, and f128 comparison I32-to-I1 bridging. Add or strengthen focused assertions only where the existing Step 3 coverage leaves a real gap.

## Watchouts

- The prepared-printer helper file intentionally does not call the new predicates because some lightweight printer-only tests link printer objects without the runtime-helper fact-population objects.
- AArch64 i128 helper boundary support still consumes div/rem helper records directly; i128 conversion helper ABI is now named in the prepared contract predicate but is not broadened into unrelated AArch64 lowering.
- Keep unsupported or mismatched helper families fail-closed. Do not replace the named predicates with callee-name parsing or testcase-shaped dispatch guesses.

## Proof

Passed:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepared_printer|backend_aarch64_target_instruction_records|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer)$') > test_after.log 2>&1`

`test_after.log`: build completed and 5/5 targeted backend tests passed.
