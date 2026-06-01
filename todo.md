Status: Active
Source Idea Path: ideas/open/72_aarch64_special_carrier_prepared_policy_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Consume Prepared I128 Carrier And Lane Facts

# Current Packet

## Just Finished

Completed `plan.md` Step 2 for the non-helper AArch64 i128 pair/shift/compare
path in `src/backend/mir/aarch64/codegen/i128_ops.cpp`.

The non-helper result/lhs/rhs operand construction now routes through one local
`PreparedI128PairCarrierLaneAdapter` before building pair operand records. The
adapter consumes the prepared carrier pointer plus prepared low/high lane
roles, indexes, widths, and register names, then hands target-local record
construction converted AArch64 register operands. Opcode selection, shift-count
handling, compare sequence policy, and final pair/shift/compare machine-record
emission stayed AArch64-local.

## Suggested Next

Next coherent packet: Step 3 should narrow to the i128 div/rem helper boundary
in `src/backend/mir/aarch64/codegen/i128_ops.cpp` and consume prepared
runtime-helper, ABI, preservation, clobber, selected-call ownership, and
marshaling facts without moving AArch64 helper-boundary record construction or
final `bl` emission out of target-local code.

## Watchouts

`clang-format` was not available in this environment, so the touched code was
left in the surrounding manual style. The helper-boundary path still has its
own operand reconstruction and policy checks; that was intentionally left for
Step 3.

## Proof

Delegated proof passed:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_target_instruction_records$' >> test_after.log 2>&1`.
Proof log: `test_after.log`.
