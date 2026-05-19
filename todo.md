Status: Active
Source Idea Path: ideas/open/318_aarch64_scalar_alu_immediate_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Classify Residuals

# Current Packet

## Just Finished

Step 4 - Validate And Classify Residuals is complete. The remaining
`00204.c` failure is outside idea 318's scalar ALU immediate materialization
owner.

Evidence:

- The focused proof advances past the former assembler diagnostic. The
  generated `subim503808` site no longer contains illegal `mov w9, #503808`;
  it contains legal `movz w9, #45056` and `movk w9, #7, lsl #16` before
  `sub w0, w0, w9`.
- `backend_aarch64_machine_printer`, `backend_aarch64_instruction_dispatch`,
  `backend_aarch64_target_instruction_records`, and the focused `00204`
  BIR/prepared-BIR guardrails all pass.
- The remaining failure is runtime-only:
  `c_testsuite_aarch64_backend_src_00204_c` reports `RUNTIME_NONZERO` with
  `exit=Segmentation fault`.
- The observed stdout reaches only the `Arguments:` section and shows
  corrupted HFA floating/long-double argument output before the crash. It does
  not reach `Return values:`, `stdarg:`, `MOVI:`, or the `opi()` scalar
  arithmetic section.
- `subim503808` is called only from `opi()`, after `arg()`, `ret()`,
  `stdarg()`, and `movi()` in `pcs()`. Therefore the runtime residual occurs
  before the repaired scalar ALU representative function is executed.

## Suggested Next

Ask the plan owner to close idea 318 as complete and create or activate a
follow-up owner for the `00204.c` runtime argument/HFA corruption plus
segfault. The current evidence points at AArch64 ABI argument handling for
HFA/aggregate values, not scalar ALU immediate materialization.

## Watchouts

- Do not keep idea 318 open for the runtime segfault unless new generated-code
  evidence shows a scalar ALU immediate materialization fault still reaches
  executable code.
- The follow-up should start from the earliest observed runtime corruption in
  the `Arguments:` HFA/aggregate path, not from `subim503808` or `opi()`.
- The current proof does not show a `va_start` helper-text, large frame
  adjustment, stack-slot spelling, or frame-layout first bad fact.
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
failure is `c_testsuite_aarch64_backend_src_00204_c`, advanced from the
baseline assembler error to runtime `RUNTIME_NONZERO` with `exit=Segmentation
fault`.

Confirmed generated assembly at
`build/c_testsuite_aarch64_backend/src/00204.c.s:8502` no longer contains
`mov w9, #503808`; it contains `movz w9, #45056` plus
`movk w9, #7, lsl #16`.

Classification: outside idea 318. The scalar ALU acceptance condition is met;
the remaining first bad fact belongs to a separate runtime ABI/HFA/aggregate
argument owner.
