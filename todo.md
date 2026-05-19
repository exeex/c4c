Status: Active
Source Idea Path: ideas/open/318_aarch64_scalar_alu_immediate_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Scalar ALU Immediate Path

# Current Packet

## Just Finished

Step 1 - Localize Scalar ALU Immediate Path is complete. The missing fact is
not operand selection or scratch-register choice; it is scalar ALU constant
publication for non-encodable immediates. `00204.c` reaches `subim503808` with
structured scalar add/sub operands and prints:

```asm
mov w0, #0
mov w9, #503808
sub w0, w0, w9
sub w0, w0, w0
```

The illegal line is emitted by `src/backend/mir/aarch64/codegen/alu.cpp` inside
`make_scalar_alu_print_lines`, specifically the local immediate
materialization helpers for scalar integer ALU sources:

- `materialize_source(...)` for immediate lhs or general immediate sources
- `materialize_rhs_immediate_for_existing_lhs(...)` for non-encodable rhs
  immediates
- `materialize_lhs_immediate_into_result(...)` for immediate/immediate pairs

Those paths already distinguish direct encodable add/sub immediates via
`scalar_alu_operation_accepts_immediate(...)`, but the fallback emits a single
`mov <scratch-or-result>, #<imm>` line. That is legal for the existing local
small cases such as `4096`, but not for larger constants such as `503808`.
The repair should reuse the existing AArch64 constant sequence helper
`materialize_integer_constant_lines(...)` instead of printing a raw `mov`
fallback.

## Suggested Next

Implement Step 2 - Materialize Non-Encodable ALU Constants in
`src/backend/mir/aarch64/codegen/alu.cpp`. Keep direct encodable add/sub
immediates on the current `#imm` path, but route non-encodable scalar ALU
fallback constants through `materialize_integer_constant_lines(...)` before
the ALU operation.

## Watchouts

- Representative local coverage should extend
  `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`
  `selected_scalar_add_sub_materializes_nonencodable_immediates` or a nearby
  scalar ALU printer test with a value requiring multiple move-wide chunks
  such as `503808`, while preserving direct encodable immediate behavior from
  `selected_scalar_add_with_immediate_operands_prints_structured_add`.
- `backend_aarch64_prepared_scalar_alu_records` already covers rematerialized
  immediate source identity; the implementation packet should not alter that
  selection contract unless the printer evidence proves it is necessary.
- `backend_aarch64_instruction_dispatch` and the focused `00204` prepared/BIR
  dump tests are guardrails for upstream lowering and prior owners, not the
  primary code surface for this residual.
- Do not reopen idea 317's raw `va_start` helper-text lowering owner.
- Do not reopen idea 315's large frame setup/teardown materialization owner.
- Do not reopen idea 314's stack-slot memory or scalar stack-publication owner.
- Do not fold idea 316's frame-slot/frame-layout consistency residual into
  this owner.
- Do not special-case `00204.c`, `subim503808`, literal `503808`, one
  scratch register, one diagnostic, or one emitted line.
- Do not change expectations, unsupported classifications, runners, timeout
  policy, CTest registration, proof-log policy, or prepared handoff contracts.

## Proof

Ran the delegated focused proof and wrote it to `test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'
```

Result: build succeeded; 11 tests ran; 10 passed. The only failure remains
`c_testsuite_aarch64_backend_src_00204_c`, with the baseline assembler
diagnostic at `build/c_testsuite_aarch64_backend/src/00204.c.s:8502`:
`mov w9, #503808`.
