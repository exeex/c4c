Status: Active
Source Idea Path: ideas/open/82_aarch64_instruction_printer_surface_contraction.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Table-Drive Local Printer Mnemonics Where Clear

# Current Packet

## Just Finished

Step 4 bounded contraction completed for one AArch64-local printer mnemonic
dispatch cluster. `machine_printer.cpp` now table-drives the repeated atomic
width-to-mnemonic helpers through a local `width_mnemonic` lookup while keeping
the same unsupported-width empty spelling behavior and emitted assembly.

## Suggested Next

Supervisor review of the Step 4 atomic mnemonic table contraction against the
source idea and active plan. If accepted, the next coherent packet is the next
bounded contraction selected by the supervisor, or plan-owner review if Step 4
exhausts the current runbook.

## Watchouts

This packet intentionally did not change public instruction/printer headers,
instruction records, tests, unsupported-path contracts, or emitted assembly.
The contraction was kept to the atomic width mnemonic helpers because that
cluster had identical width dispatch shapes and the resulting table was more
auditable than the repeated branches.

## Proof

Passed. Ran the supervisor-selected proof command:
`bash -o pipefail -c "(cmake --build build --target c4c_backend backend_aarch64_machine_printer_test backend_aarch64_target_instruction_records_test c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_(machine_printer|target_instruction_records)' --output-on-failure) 2>&1 | tee test_after.log"`.
Proof log: `test_after.log`.
