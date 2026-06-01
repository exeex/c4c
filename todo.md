Status: Active
Source Idea Path: ideas/open/82_aarch64_instruction_printer_surface_contraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Contract Instruction Record Naming And Status Helpers

# Current Packet

## Just Finished

Step 2 bounded contraction completed for instruction selection-status record
construction: repeated `MachineNodeStatusRecord` initializers in
`instruction.cpp` now delegate through local helper builders for selected,
deferred-unsupported, missing-required-facts, and generic status records.
Those helper builders have internal linkage; existing status enum values,
public API functions, and diagnostic strings were preserved.

## Suggested Next

Continue Step 2 with a separate bounded packet for another instruction naming
or printer-surface contraction that preserves currently used public diagnostics
APIs and call sites.

## Watchouts

The originally suggested printer-local API move was deferred because supervisor
call-site checking found public mnemonic helpers are still externally used. Do
not remove or rename `MachinePrinterMnemonicKind`,
`machine_printer_mnemonic_kind_name`,
`machine_opcode_printer_mnemonic_kind`,
`machine_pseudo_printer_mnemonic_kind`, or the
`machine_instruction_*printer_mnemonic*` helpers until those public call sites
are retired. The opcode and pseudo mapping tables intentionally list only
non-`None` mnemonic mappings; missing entries are still the unsupported/default
path. The new status helpers are internal to `instruction.cpp`; do not move
them to the public header unless a future packet explicitly owns that API
change.

## Proof

Passed. Ran the supervisor-selected proof command:
`bash -o pipefail -c "(cmake --build build --target c4c_backend backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test backend_aarch64_instruction_dispatch_test c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)' --output-on-failure) 2>&1 | tee test_after.log"`.
Proof log: `test_after.log`.
