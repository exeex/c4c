Status: Active
Source Idea Path: ideas/open/222_aarch64_scalar_return_alu_selected_nodes.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Select Returned Scalar Add/Sub Machine Nodes

# Current Packet

## Just Finished

Step 3 implementation completed for `plan.md` Step 3, `Select Returned Scalar Add/Sub Machine Nodes`.

Implemented prepared-module selection for returned scalar add/sub:
- Added AArch64 module-machine construction for simple returned BIR `Add`/`Sub` instructions using existing prepared scalar ALU record conversion.
- Emits selected scalar `MachineOpcode::Add`/`Sub` nodes with prepared lhs/rhs register uses, destination register defs, function/block labels, and source instruction index.
- Updated return-node construction to consume the structured prepared scalar result register when the returned value is produced by a selected scalar node, preserving the generic prepared-value fallback for other returns.
- Added focused prepared-module identity coverage for returned I64 add and I32 sub sequencing: scalar node first, return node second, with register uses/defs and no generic prepared-value placeholder for the scalar return.

Kept out of scope as required:
- Did not edit `machine_printer.cpp`.
- Did not enable `tests/backend/case/return_add.c`.
- Did not edit backend case smoke wiring or `tests/backend/CMakeLists.txt`.

## Suggested Next

Delegate the next packet to add printer spelling and print coverage for selected scalar add/sub using the structured destination/result register facts now emitted through the prepared-module machine-node path.

## Watchouts

- Do not enable `tests/backend/case/return_add.c` before printer coverage exists for selected scalar add/sub plus returned scalar results.
- Reject named-case matching, fixture assembly text, expectation downgrades, or unsupported marking as progress.
- Keep `return_add_sub_chain.c` and broader scalar ALU cases deferred unless they receive matching structured proof.
- The next printer packet should consume `ScalarInstructionRecord::result_register`; current `machine_printer.cpp` still intentionally fails selected scalar nodes closed.
- Watch for `I32` versus `I64` register views (`w` versus `x`) when printing destination and operand registers.

## Proof

Ran the supervisor-delegated Step 3 narrow proof exactly:

```sh
bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "backend_aarch64_(prepared_module_identity|prepared_scalar_alu_records|target_instruction_records)$"' > test_after.log 2>&1
```

Result: passed; build plus 3 focused CTest tests passed for `backend_aarch64_(prepared_module_identity|prepared_scalar_alu_records|target_instruction_records)$`. Output is preserved in `test_after.log`.

Supervisor-side broader validation then replaced `test_after.log` with:

```sh
bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_aarch64_"' > test_after.log 2>&1
```

Result: passed; 24/24 `backend_aarch64_` tests passed. The broader validation output is now preserved in `test_after.log`.
