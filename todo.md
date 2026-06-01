Status: Active
Source Idea Path: ideas/open/82_aarch64_instruction_printer_surface_contraction.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Table-Drive Local Printer Mnemonics Where Clear

# Current Packet

## Just Finished

Step 4 bounded contraction continued for the remaining clear AArch64-local
memory width mnemonic helpers. `machine_printer.cpp` now table-drives
`stack_publication_store_mnemonic`, `stack_source_load_mnemonic`, and
`scalar_memory_mnemonic` through explicit `width_mnemonic` mappings while
preserving unsupported-width empty spelling behavior and emitted assembly.

## Suggested Next

Supervisor review of the Step 4 memory mnemonic table contraction against the
source idea and active plan. If accepted, the next coherent packet is either
another supervisor-selected clear local printer contraction or plan-owner
review if Step 4 is exhausted.

## Watchouts

This packet intentionally did not change public instruction/printer headers,
instruction records, tests, unsupported-path contracts, or emitted assembly.
The `width_mnemonic` helper moved earlier in `machine_printer.cpp` so the stack
publication/source helpers can share it; the mapping body itself is unchanged.

## Proof

Passed. Ran the supervisor-selected proof command:
`bash -o pipefail -c "(cmake --build build --target c4c_backend backend_aarch64_machine_printer_test backend_aarch64_target_instruction_records_test c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_(machine_printer|target_instruction_records)' --output-on-failure) 2>&1 | tee test_after.log"`.
Proof log: `test_after.log`.
