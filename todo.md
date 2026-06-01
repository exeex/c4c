Status: Active
Source Idea Path: ideas/open/82_aarch64_instruction_printer_surface_contraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Contract Instruction Record Naming And Status Helpers

# Current Packet

## Just Finished

Step 2 bounded contraction completed for AArch64 prepared scalar ALU and scalar
cast record error naming: `prepared_scalar_alu_record_error_name` and
`prepared_scalar_cast_record_error_name` now delegate through internal lookup
helpers backed by explicit target-local spelling tables. All previous public
spellings were preserved, and missing/unknown enum values still return
`unknown`.

## Suggested Next

Continue Step 2 with a separate bounded packet for another same-shaped
prepared-record error naming surface in `instruction.cpp`, such as the scalar
FP unary intrinsic or address materialization record error naming helper,
preserving all public strings and fallback behavior.

## Watchouts

The ALU and cast spelling tables intentionally mirror the previous switch arms
one-for-one, including the shared scalar operand/result error spelling set and
the `register_conversion_failed` tail entry. This packet only changed the local
naming helper shape; no diagnostics, tests, or downstream instruction selection
behavior were changed.

## Proof

Passed. Ran the supervisor-selected proof command:
`bash -o pipefail -c "(cmake --build build --target c4c_backend backend_aarch64_prepared_scalar_alu_records_test backend_aarch64_prepared_scalar_cast_records_test backend_aarch64_target_instruction_records_test c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_(prepared_scalar_alu_records|prepared_scalar_cast_records|target_instruction_records)' --output-on-failure) 2>&1 | tee test_after.log"`.
Proof log: `test_after.log`.
