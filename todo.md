Status: Active
Source Idea Path: ideas/open/82_aarch64_instruction_printer_surface_contraction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Contract Printer Validation Helpers

# Current Packet

## Just Finished

Step 3 bounded contraction completed for the repeated AArch64 machine-printer
validation diagnostic shape that prefixes unsupported-path messages with the
instruction header. `machine_printer.cpp` now uses a local
`target_unsupported(instruction, diagnostic)` wrapper across assembler,
spill/reload, branch, symbol-memory, and scalar-memory validation sites. The
touched call sites still state the same concrete record or operand contract,
and diagnostic text plus printed assembly behavior were preserved.

## Suggested Next

Supervisor review of the Step 3 contraction against the source idea. If
accepted, the next coherent packet is Step 4 table-driving of local printer
mnemonic/spelling dispatch only where the table is more auditable than the
existing branches.

## Watchouts

This packet intentionally did not change `machine_printer.hpp`, tests,
instruction records, unsupported-path contracts, or emitted assembly. Some
dynamic diagnostic constructions still use the original string-building form;
the contraction was kept to same-shaped fixed validation diagnostics rather
than broad mechanical churn.

## Proof

Passed. Ran the supervisor-selected proof command:
`bash -o pipefail -c "(cmake --build build --target c4c_backend backend_aarch64_machine_printer_test c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_machine_printer' --output-on-failure) 2>&1 | tee test_after.log"`.
Proof log: `test_after.log`.
