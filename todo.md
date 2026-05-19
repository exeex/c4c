Status: Active
Source Idea Path: ideas/open/318_aarch64_scalar_alu_immediate_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Materialize Non-Encodable ALU Constants

# Current Packet

## Just Finished

Step 2 - Materialize Non-Encodable ALU Constants is complete. Scalar ALU
fallback constants in `src/backend/mir/aarch64/codegen/alu.cpp` now preserve
the existing direct `mov` path for small printable constants, but route larger
fallback immediates through `materialize_integer_constant_lines(...)`.

The implemented owner surface is `make_scalar_alu_print_lines`, specifically:

- `materialize_source(...)` for immediate lhs or general immediate sources
- `materialize_rhs_immediate_for_existing_lhs(...)` for non-encodable rhs
  immediates
- `materialize_lhs_immediate_into_result(...)` for immediate/immediate pairs

Focused local coverage was added to
`tests/backend/mir/backend_aarch64_machine_printer_test.cpp` in
`selected_scalar_add_sub_materializes_nonencodable_immediates`. It now proves a
multi-chunk scalar ALU fallback constant materializes as `movz`/`movk` before
the ALU operation, while existing nearby coverage preserves direct encodable
immediate behavior such as `add w0, w0, #3` and small fallback `mov` cases.

The former `00204.c` bad site now prints legal materialization:

```asm
mov w0, #0
movz w9, #45056
movk w9, #7, lsl #16
sub w0, w0, w9
sub w0, w0, w0
```

## Suggested Next

Run Step 4 - Validate And Classify Residuals. The scalar ALU assembler blocker
is gone; the focused representative now advances to runtime execution and
fails with `RUNTIME_NONZERO` / `Segmentation fault`. Classify that first bad
runtime fact before widening into any new implementation owner.

## Watchouts

- The current focused residual is no longer an assembler diagnostic. It is a
  runtime segmentation fault from `c_testsuite_aarch64_backend_src_00204_c`
  after printing many arguments and floating values.
- Do not treat the runtime failure as scalar ALU immediate materialization
  without generated-code evidence. The former illegal line at `subim503808`
  has advanced to legal `movz`/`movk`.
- `backend_aarch64_prepared_scalar_alu_records`, `backend_aarch64_instruction_dispatch`,
  and the focused `00204` prepared/BIR dump tests remain guardrails for
  upstream lowering and prior owners, not proof of ownership for the runtime
  residual.
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

Result: build succeeded; 11 tests ran; 10 passed. The local backend guardrails,
including `backend_aarch64_machine_printer`, passed. The only remaining
failure is `c_testsuite_aarch64_backend_src_00204_c`, now advanced from the
baseline assembler error to runtime `RUNTIME_NONZERO` with `exit=Segmentation
fault`.

Confirmed generated assembly at
`build/c_testsuite_aarch64_backend/src/00204.c.s:8502` no longer contains
`mov w9, #503808`; it contains `movz w9, #45056` plus
`movk w9, #7, lsl #16`.
