Status: Active
Source Idea Path: ideas/open/82_aarch64_instruction_printer_surface_contraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Contract Instruction Record Naming And Status Helpers

# Current Packet

## Just Finished

Step 2 bounded contraction completed for AArch64 prepared address
materialization record error naming:
`prepared_address_materialization_record_error_name` now delegates through an
internal lookup helper backed by an explicit target-local spelling table. All
previous public spellings were preserved, and missing/unknown enum values still
return `unknown`.

## Suggested Next

Continue Step 2 by reviewing the remaining instruction record naming and status
helpers in `instruction.cpp` before moving to Step 3; if another direct public
switch still exposes a spelling table, contract it in one bounded packet while
preserving all public strings and fallback behavior.

## Watchouts

The address materialization spelling table intentionally mirrors the previous
switch arms one-for-one, including policy mismatch, TLS fact mismatch, and the
`register_conversion_failed` entry. This packet only changed the local naming
helper shape; no diagnostics, tests, or downstream instruction selection
behavior were changed.

## Proof

Passed. Ran the supervisor-selected proof command:
`bash -o pipefail -c "(cmake --build build --target c4c_backend backend_aarch64_prepared_memory_operand_records_test backend_aarch64_target_instruction_records_test c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_(prepared_memory_operand_records|target_instruction_records)' --output-on-failure) 2>&1 | tee test_after.log"`.
Proof log: `test_after.log`.
