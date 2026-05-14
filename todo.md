Status: Active
Source Idea Path: ideas/open/222_aarch64_scalar_return_alu_selected_nodes.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Markdown Contracts And Existing AArch64 ALU/Return Surfaces

# Current Packet

## Just Finished

Step 1 inspection completed for `plan.md` Step 1, `Inspect Markdown Contracts And Existing AArch64 ALU/Return Surfaces`.

Exact markdown obligations:
- `ideas/open/222_aarch64_scalar_return_alu_selected_nodes.md` requires the route `alu.md` plus `returns.md` -> `records.cpp/.hpp` -> selected scalar machine-node/operator coverage -> `machine_printer.cpp` -> `tests/backend/case/return_add.c`.
- `src/backend/mir/aarch64/codegen/alu.md` requires integer ALU lowering to select typed target MIR operations and then machine instruction nodes before final mnemonic spelling; add/sub must remain semantic scalar ALU lowering, not printer text or testcase matching.
- `src/backend/mir/aarch64/codegen/returns.md` requires return lowering to publish structured return facts and machine instruction nodes for return-value movement and control transfer; final `ret` and register spelling belong to printer/encoding consumers.
- Public `return_add.c` must stay disabled until structured selected-node/operator and printer coverage proves scalar add plus returned-result emission.

Code owners and current surfaces:
- `src/backend/mir/aarch64/codegen/records.hpp` owns `ScalarAluOperationKind::{Add, Sub, And, Or, Xor, Deferred}`, `ScalarAluRecord`, `ScalarInstructionRecord`, `ReturnInstructionRecord`, `MachineOpcode::{Add, Sub, ...}`, and `MachinePrinterMnemonicKind`, but printer mnemonics currently omit add/sub.
- `src/backend/mir/aarch64/codegen/records.cpp` maps BIR add/sub to scalar ALU operations, maps scalar ALU add/sub to `MachineOpcode::Add`/`MachineOpcode::Sub`, marks supported scalar ALU records selected, and builds selected return instruction records with return/control-flow side effects.
- `src/backend/mir/aarch64/codegen/machine_printer.cpp` prints spill/reload, branch, store, and immediate-return nodes; selected scalar nodes currently fail closed with `scalar node is missing a structured destination register operand`, so add/sub text is not printable yet.

Current focused tests and public smoke status:
- Existing focused C++ tests already cover scalar ALU records and prepared scalar ALU conversion in `backend_aarch64_scalar_alu_records` and `backend_aarch64_prepared_scalar_alu_records`.
- `backend_aarch64_target_instruction_records` covers scalar instruction records and return placeholders, plus existing mnemonic spelling for branch/load/store/mov/ret.
- `backend_aarch64_machine_printer` covers selected spill/reload, branch/store, immediate `mov` + `ret`, and fail-closed scalar printing.
- `tests/backend/case/return_add.c` exists and contains `int main() { return 2 + 3; }`, but no `return_add` CTest registration was found; the enabled AArch64 external smoke remains `backend_cli_aarch64_asm_external_return_zero_smoke` for `aarch64_return_zero_smoke.c`.

Gaps:
- No structured destination register/result operand is carried on scalar ALU nodes, so the machine printer cannot emit add/sub.
- `MachinePrinterMnemonicKind` and `machine_opcode_printer_mnemonic_kind` do not spell add/sub.
- Return records can print only immediate return values; there is no focused coverage for returning a previously selected scalar result through `x0`/`w0`.
- The public `return_add.c` smoke is intentionally not enabled and should remain deferred until record, selected-node/operator, and printer coverage exist.

## Suggested Next

Delegate Step 2 as a records-and-focused-tests packet:

Owned files should be `src/backend/mir/aarch64/codegen/records.hpp`, `src/backend/mir/aarch64/codegen/records.cpp`, `tests/backend/backend_aarch64_scalar_alu_records_test.cpp`, `tests/backend/backend_aarch64_target_instruction_records_test.cpp`, and `tests/backend/backend_aarch64_prepared_scalar_alu_records_test.cpp`.

Recommended implementation: extend the scalar ALU record/instruction shape with a structured destination/result register operand derived from prepared storage for add/sub, preserve existing prepared value identities and `MachineOpcode::Add`/`Sub`, and add focused tests proving add/sub selected machine nodes carry lhs/rhs plus destination register facts and return records can reference a prepared scalar result without printing or enabling the public case yet.

Recommended narrow proof target after build: `ctest --test-dir build -R 'backend_aarch64_(scalar_alu_records|prepared_scalar_alu_records|target_instruction_records)$' --output-on-failure`.

## Watchouts

- Do not enable `tests/backend/case/return_add.c` before structured record, selected-node/operator, and printer coverage exists.
- Reject named-case matching, fixture assembly text, expectation downgrades, or unsupported marking as progress.
- Keep `return_add_sub_chain.c` and broader scalar ALU cases deferred unless they receive matching structured proof.
- Step 2 should not edit `machine_printer.cpp` unless the supervisor deliberately folds printer support into the packet; the clean first cut is to establish the scalar destination/return-result record contract that Step 4 can print.
- Watch for `I32` versus `I64` register views (`w` versus `x`) when converting prepared scalar result storage; current tests already enforce view mismatches fail closed.

## Proof

Ran the supervisor-delegated inspection proof exactly:

```sh
bash -lc 'set -o pipefail; { rg -n "ScalarInstructionRecord|ScalarAluOperationKind|ReturnInstructionRecord|MachinePrinterMnemonicKind|MachineOpcode::(Add|Sub|Branch)|return_add|aarch64|add|sub|ret|selected|machine node|machine-node" src/backend/mir/aarch64 tests/backend tests/backend/case/return_add.c CMakeLists.txt -g "*.md" -g "*.cpp" -g "*.hpp" -g "CMakeLists.txt"; } > test_after.log 2>&1; test -s test_after.log'
```

Result: passed; inspection output is preserved in `test_after.log`. This is sufficient for the inspection-only packet and intentionally does not include build/test execution.
