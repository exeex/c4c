Status: Active
Source Idea Path: ideas/open/82_aarch64_instruction_printer_surface_contraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Contract Instruction Record Naming And Status Helpers

# Current Packet

## Just Finished

Step 2 bounded contraction completed for `MachinePrinterMnemonicKind` spelling:
`machine_printer_mnemonic_kind_name` now reads from an explicit local table and
lookup helper in `instruction.cpp` instead of a repeated spelling switch.
Public API symbols and return strings were preserved.

## Suggested Next

Continue Step 2 with a separate bounded packet for another instruction naming
or status-helper contraction that preserves currently used public diagnostics
APIs.

## Watchouts

The originally suggested printer-local API move was deferred because supervisor
call-site checking found public mnemonic helpers are still externally used. Do
not remove or rename `MachinePrinterMnemonicKind`,
`machine_printer_mnemonic_kind_name`,
`machine_opcode_printer_mnemonic_kind`,
`machine_pseudo_printer_mnemonic_kind`, or the
`machine_instruction_*printer_mnemonic*` helpers until those public call sites
are retired.

## Proof

Passed. Ran the supervisor-selected proof command:
`bash -o pipefail -c "(cmake --build build --target c4c_backend backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test backend_aarch64_instruction_dispatch_test c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)' --output-on-failure) 2>&1 | tee test_after.log"`.
Proof log: `test_after.log`.
