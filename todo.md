Status: Active
Source Idea Path: ideas/open/222_aarch64_scalar_return_alu_selected_nodes.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Print Structured Add/Sub And Return Assembly

# Current Packet

## Just Finished

Step 4 implementation completed for `plan.md` Step 4, `Print Structured Add/Sub And Return Assembly`.

Implemented focused machine-printer and mnemonic coverage for selected scalar add/sub:
- Added central `MachinePrinterMnemonicKind::Add`/`Sub` spellings and mapped `MachineOpcode::Add`/`Sub` to `add`/`sub`.
- Printed selected scalar add/sub machine nodes from `ScalarInstructionRecord::result_register` and two structured register inputs.
- Kept scalar printing fail-closed for missing destination registers, non-add/sub scalar opcodes, wrong input count, and non-register scalar operands.
- Updated register-valued return nodes to print only `ret`; immediate returns still emit the existing `mov` plus `ret` sequence.
- Added focused machine-printer and target-instruction-record tests for add/sub mnemonics, x/w register operand spelling, register-valued `ret`, and scalar fail-closed behavior.

Kept out of scope as required:
- Did not enable `tests/backend/case/return_add.c`.
- Did not edit backend CMake public smoke wiring.
- Did not touch `plan.md` or source idea files.

## Suggested Next

Delegate the next coherent packet to connect the existing prepared-module selected add/sub nodes to any non-public focused assembly path the supervisor wants, or to request plan-owner/reviewer assessment if Step 4 completes the current runbook slice.

## Watchouts

- Printer coverage now exists for selected scalar add/sub plus register-valued return nodes, but do not enable `tests/backend/case/return_add.c` unless the supervisor explicitly delegates public smoke wiring.
- Reject named-case matching, fixture assembly text, expectation downgrades, or unsupported marking as progress.
- Keep `return_add_sub_chain.c` and broader scalar ALU cases deferred unless they receive matching structured proof.
- The current printer intentionally supports only register-register selected scalar add/sub; immediate-form scalar ALU printing remains fail-closed.
- Watch for `I32` versus `I64` register views (`w` versus `x`) when expanding beyond the covered scalar register-register subset.

## Proof

Ran the supervisor-delegated Step 4 narrow proof exactly:

```sh
bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "backend_aarch64_(machine_printer|target_instruction_records|prepared_module_identity)$"' > test_after.log 2>&1
```

Result: passed; build plus 3 focused CTest tests passed for `backend_aarch64_(machine_printer|target_instruction_records|prepared_module_identity)$`.

Supervisor-side broader validation now preserved in `test_after.log`:

```sh
bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_aarch64_"' > test_after.log 2>&1
```

Result: passed; 24/24 `backend_aarch64_` tests passed. The broader validation output is now preserved in `test_after.log`.
