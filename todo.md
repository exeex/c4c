Status: Active
Source Idea Path: ideas/open/222_aarch64_scalar_return_alu_selected_nodes.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Scalar ALU And Return Record Coverage

# Current Packet

## Just Finished

Step 2 implementation completed for `plan.md` Step 2, `Add Scalar ALU And Return Record Coverage`.

Implemented record-layer coverage only:
- Added optional structured `result_register` facts to `ScalarAluRecord` and `ScalarInstructionRecord`.
- Converted prepared scalar ALU result storage into a typed AArch64 `RegisterOperand` with prepared value id/name, bank/class, view, contiguous width, and occupied register names.
- Propagated scalar result registers into selected scalar machine-node defs while preserving the existing prepared-value fallback for records without a register fact.
- Added focused add/sub coverage proving selected scalar machine nodes keep lhs/rhs uses and a destination register def.
- Added return-record coverage proving a return node can reference a prepared scalar result register.

Kept out of scope as required:
- Did not edit `machine_printer.cpp`.
- Did not enable `tests/backend/case/return_add.c`.
- Did not edit backend case smoke wiring or `tests/backend/CMakeLists.txt`.

## Suggested Next

Delegate the next packet to add printer spelling and print coverage for selected scalar add/sub using the structured destination/result register facts now carried by scalar ALU records.

## Watchouts

- Do not enable `tests/backend/case/return_add.c` before printer coverage exists for selected scalar add/sub plus returned scalar results.
- Reject named-case matching, fixture assembly text, expectation downgrades, or unsupported marking as progress.
- Keep `return_add_sub_chain.c` and broader scalar ALU cases deferred unless they receive matching structured proof.
- The next printer packet should consume `ScalarInstructionRecord::result_register`; current `machine_printer.cpp` still intentionally fails selected scalar nodes closed.
- Watch for `I32` versus `I64` register views (`w` versus `x`) when printing destination and operand registers.

## Proof

Ran the supervisor-delegated Step 2 narrow proof exactly:

```sh
bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "backend_aarch64_(scalar_alu_records|prepared_scalar_alu_records|target_instruction_records)$"' > test_after.log 2>&1
```

Result: passed; build plus 3 focused CTest tests passed for `backend_aarch64_(scalar_alu_records|prepared_scalar_alu_records|target_instruction_records)$`.

Supervisor-side broader validation then replaced `test_after.log` with:

```sh
bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_aarch64_"' > test_after.log 2>&1
```

Result: passed; 24/24 `backend_aarch64_` tests passed. The broader validation output is now preserved in `test_after.log`.
