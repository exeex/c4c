Status: Active
Source Idea Path: ideas/open/82_aarch64_instruction_printer_surface_contraction.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Table-Drive Local Printer Mnemonics Where Clear

# Current Packet

## Just Finished

Step 4 bounded contraction continued for the remaining clear aggregate
width-to-mnemonic helpers in `instruction.cpp`. `aggregate_stack_copy_load_mnemonic`,
`aggregate_stack_copy_store_mnemonic`, and
`aggregate_register_lane_load_mnemonic` now table-drive their explicit
AArch64-local width mappings while preserving unsupported-width empty spelling
behavior and emitted assembly.

## Suggested Next

Supervisor review of the Step 4 memory mnemonic table contraction against the
source idea and active plan. If accepted, the next coherent packet is either
another supervisor-selected clear local printer contraction or plan-owner
review if Step 4 is exhausted.

## Watchouts

This packet intentionally did not change public instruction/printer headers,
instruction records, tests, unsupported-path contracts, or emitted assembly.
The new aggregate mnemonic tables are file-local to `instruction.cpp` and keep
the prior supported width sets exactly: stack copy load/store support 1, 4, and
8 bytes; register-lane loads support 1, 2, 4, and 8 bytes.

## Proof

Passed. Ran the supervisor-selected proof command:
`bash -o pipefail -c "(cmake --build build --target c4c_backend backend_aarch64_machine_printer_test backend_aarch64_target_instruction_records_test c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_(machine_printer|target_instruction_records)' --output-on-failure) 2>&1 | tee test_after.log"`.
Proof log: `test_after.log`.
