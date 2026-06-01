Status: Active
Source Idea Path: ideas/open/82_aarch64_instruction_printer_surface_contraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Contract Instruction Record Naming And Status Helpers

# Current Packet

## Just Finished

Step 2 bounded contraction completed for AArch64 machine opcode record naming:
the large `machine_opcode_name` switch in `instruction.cpp` now delegates
through an internal `machine_opcode_spelling` lookup helper backed by an
explicit target-local `kMachineOpcodeSpellings` table. All previous opcode
spellings were preserved, and missing/unknown opcodes still return `unknown`.

## Suggested Next

Continue Step 2 with a separate bounded packet for another local naming/status
surface contraction that preserves public diagnostics APIs and call sites.

## Watchouts

The opcode spelling table intentionally includes every current `MachineOpcode`
entry from the previous switch. The helper keeps the default `unknown` path for
out-of-range or unmapped enum values. Public APIs and printer mnemonic helpers
remain untouched.

## Proof

Passed. Ran the supervisor-selected proof command:
`bash -o pipefail -c "(cmake --build build --target c4c_backend backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test backend_aarch64_instruction_dispatch_test c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)' --output-on-failure) 2>&1 | tee test_after.log"`.
Proof log: `test_after.log`.
