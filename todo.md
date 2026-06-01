Status: Active
Source Idea Path: ideas/open/82_aarch64_instruction_printer_surface_contraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Contract Instruction Record Naming And Status Helpers

# Current Packet

## Just Finished

Step 2 bounded contraction completed for the remaining small direct public enum
name switches in `instruction.cpp`: `operand_kind_name`,
`register_operand_role_name`, `instruction_family_name`, and
`address_materialization_kind_name` now delegate through internal lookup helpers
backed by explicit target-local spelling tables. All previous public spellings
were preserved, and missing/unknown enum values still return `unknown`.

## Suggested Next

Step 2 appears ready for supervisor review before Step 3. Recommended next
packet: supervisor-side review of the Step 2 diff against the source idea and
then either transition to Step 3 or delegate any review-found cleanup.

## Watchouts

The new spelling tables intentionally mirror the previous switch arms
one-for-one for operand kinds, register operand roles, instruction families, and
address materialization kinds. This packet only changed the local naming helper
shape; no diagnostics, tests, machine printer files, instruction dispatch
behavior, or downstream instruction selection behavior were changed.

## Proof

Passed. Ran the supervisor-selected proof command:
`bash -o pipefail -c "(cmake --build build --target c4c_backend backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test backend_aarch64_instruction_dispatch_test c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)' --output-on-failure) 2>&1 | tee test_after.log"`.
Proof log: `test_after.log`.
