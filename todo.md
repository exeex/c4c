# Current Packet

Status: Active
Source Idea Path: ideas/open/224_common_mir_container_and_target_printer_boundary.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Retire Or Fence Temporary Display And Flat-Printer Paths

## Just Finished

Completed `plan.md` Step 6 by retiring the AArch64 flat terminal assembly
helpers from `machine_printer.*`. AST-backed symbol checks plus repository
reference checks showed the flat `print_machine_instruction_node(s)` entrypoints
were no longer used by production code; the remaining AArch64 printer API is the
target line-payload hook consumed by the shared MIR printer.

`FunctionRecord::machine_nodes` and the compatibility projection derivation are
now explicitly fenced as compatibility-only flat views, with terminal assembly
ownership documented on `module::MachineFunction` plus the shared MIR printer.
The machine-printer test now prints selected records through a local
`mir::MachineFunction` and `MachineInstructionPrinter` instead of the removed
flat-vector helper.

## Suggested Next

Supervisor should run Step 7 broader validation and handoff/closeout review for
idea 224, because public AArch64 assembly routing now walks the shared MIR
printer and the remaining flat views are fenced as non-authoritative
compatibility projections.

## Watchouts

- No AArch64 instruction coverage was broadened; tests were rerouted to the
  shared MIR traversal boundary and still exercise the same selected spelling
  families and fail-closed diagnostics.
- `clang-format` is not installed in this environment, so formatting was kept
  manual for this slice.

## Proof

Delegated proof passed and wrote CTest output to `test_after.log`:

```bash
cmake --build build --target c4c_backend backend_aarch64_mir_carrier_test backend_aarch64_machine_printer_test c4cll && ctest --test-dir build -R '^(backend_aarch64_mir_carrier|backend_aarch64_machine_printer|backend_cli_aarch64_asm_external_return_add_smoke)$' --output-on-failure > test_after.log
```

CTest subset: `backend_aarch64_machine_printer`,
`backend_aarch64_mir_carrier`, and
`backend_cli_aarch64_asm_external_return_add_smoke`, all passed.
