Status: Active
Source Idea Path: ideas/open/257_aarch64_returns_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Move Return Construction and Lowering Bodies

# Current Packet

## Just Finished

Step 2 completed the return selected-node ownership cleanup: the construction
body and sole declaration surface now live under the returns owner.

Concrete changes:

- `returns.cpp` now owns `make_return_instruction`, including operand list
  construction, selected-node status, `Return`/`ControlFlowTransfer` side
  effects, and payload preservation.
- `returns.cpp` keeps return-use effect construction local because the existing
  broad `effects_from_operands` helper has anonymous-namespace linkage inside
  `instruction.cpp`; publishing it caused unrelated owner name conflicts.
- `instruction.cpp` no longer owns the return-family selected-node construction
  body.
- `instruction.hpp` still owns the `ReturnInstructionRecord` shape but no
  longer declares `make_return_instruction`.
- `returns.hpp` is the only header declaration surface for
  `make_return_instruction`.
- Direct test callers in target-record, target-instruction, machine-printer,
  and prepared scalar ALU coverage now include `returns.hpp`; no test
  expectations or assertions changed.
- `dispatch.cpp`, `machine_printer.cpp`, `plan.md`, and the source idea were
  not touched.

## Suggested Next

Next narrow code-moving packet: move return-specific printer spelling currently
owned by `machine_printer.cpp::print_return` into the returns owner for Step 3,
without changing the existing final assembly text or diagnostic behavior.

## Watchouts

- `machine_printer.cpp` still owns return-specific final spelling: register
  valued returns print `ret`, small signed integer immediates materialize into
  `w0` or `x0` before `ret`, and unsupported return operands/types fail closed.
- `instruction.cpp` still has return metadata classification in
  `machine_instruction_primary_printer_mnemonic_kind` and
  `machine_instruction_auxiliary_printer_mnemonic_kind`; decide whether that is
  a later ownership move or neutral metadata before changing it.
- Do not move `ReturnInstructionRecord` out of `instruction.hpp`; this slice
  preserved the record-definition boundary.
- Do not implement legacy F32/F64/F128 or second-component register moves from
  markdown text. They require structured carrier authority under
  `AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`.
- Treat any attempt to preserve second-component `s1`/`d1`/`q1` behavior by
  hard-coded register spelling as a carrier gap, not as progress.

## Proof

Supervisor-selected proof was run successfully and preserved in
`test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(return_lowering|signature_metadata|machine_printer|instruction_dispatch|target_record_core_contract|target_instruction_records|prepared_scalar_alu_records)$' > test_after.log 2>&1
```

`test_after.log` reports 7/7 tests passed:
`backend_aarch64_return_lowering`, `backend_aarch64_signature_metadata`,
`backend_aarch64_machine_printer`, `backend_aarch64_instruction_dispatch`,
`backend_aarch64_target_record_core_contract`,
`backend_aarch64_target_instruction_records`, and
`backend_aarch64_prepared_scalar_alu_records`.
