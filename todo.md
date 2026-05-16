Status: Active
Source Idea Path: ideas/open/257_aarch64_returns_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move Return-Specific Spelling Helpers

# Current Packet

## Just Finished

Step 3 completed the return-specific printer spelling helper move: the return
owner now exposes the narrow spelling-policy decisions used by
`machine_printer.cpp::print_return`, while the printer still validates operands,
formats registers, and emits the concrete assembly lines.

Concrete changes:

- `returns.hpp` now declares `ReturnValuePrintForm`,
  `classify_return_value_print_form`,
  `is_printable_return_immediate_materialization_value`, and
  `return_immediate_materialization_register`.
- `returns.cpp` owns the return-only decisions that register-valued returns
  print as the primary return, only small nonnegative signed immediates are in
  the selected printable materialization subset, and immediate materialization
  targets `w0` for I1/I8/I16/I32 or `x0` for I64.
- `machine_printer.cpp::print_return` now consumes those helpers but keeps the
  previous diagnostic text, mnemonic validation, `abi::register_name`
  formatting, and final line emission.
- No test expectations, diagnostics, emitted assembly contracts, `plan.md`, the
  source idea, `returns.md`, `instruction.cpp`, `instruction.hpp`,
  `dispatch.cpp`, or tests were changed.

## Suggested Next

Next narrow lifecycle/execution packet: start Step 4 by reconciling
`src/backend/mir/aarch64/codegen/returns.md` against the compiled returns owner
and current contracts, then delete the markdown shard only if every durable
valid item is represented in code or explicitly stale/out-of-scope.

## Watchouts

- `machine_printer.cpp` still owns generic printer validation and emission for
  returns; do not pull `bad_header`, mnemonic checks, `abi::register_name`, or
  concrete line construction into `returns.cpp` without a new ownership
  decision.
- `instruction.cpp` still has return metadata classification in
  `machine_instruction_primary_printer_mnemonic_kind` and
  `machine_instruction_auxiliary_printer_mnemonic_kind`; this slice treated it
  as neutral instruction metadata, not printer spelling.
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
