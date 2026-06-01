Status: Active
Source Idea Path: ideas/open/82_aarch64_instruction_printer_surface_contraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Contract Instruction Record Naming And Status Helpers

# Current Packet

## Just Finished

Step 2 bounded contraction completed for AArch64 record surface kind naming:
`record_surface_kind_name` now delegates through an internal
`record_surface_kind_spelling` lookup helper backed by an explicit target-local
`kRecordSurfaceKindSpellings` table. All previous record surface spellings were
preserved, and missing/unknown record surface kinds still return `unknown`.

## Suggested Next

Continue Step 2 with a separate bounded packet for another small local naming
surface such as `immediate_kind_name` or `frame_instruction_kind_name`, preserving
all public strings and `unknown` fallbacks.

## Watchouts

The record surface spelling table intentionally includes every current
`RecordSurfaceKind` entry from the previous switch. The existing record surface
classification predicates remain unchanged.

## Proof

Passed. Ran the supervisor-selected proof command:
`bash -o pipefail -c "(cmake --build build --target c4c_backend backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test backend_aarch64_instruction_dispatch_test c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)' --output-on-failure) 2>&1 | tee test_after.log"`.
Proof log: `test_after.log`.
