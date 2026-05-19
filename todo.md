Status: Active
Source Idea Path: ideas/open/317_aarch64_variadic_va_start_helper_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Lower Va Start To Legal AArch64 Assembly

# Current Packet

## Just Finished

Step 2 - Lower Va Start To Legal AArch64 Assembly complete. The
`PreparedVariadicEntryHelperKind::VaStart` printer path now consumes the
selected `VariadicVaStartRecord` and emits legal AArch64 instructions instead
of raw helper payload text.

The lowering initializes the destination `va_list` fields from the prepared
AAPCS64 layout:

- materializes `overflow_arg_area` from the prepared overflow stack base
- materializes `gp_register_save_area` as the GP register-save-area top
- materializes `fp_register_save_area` as the FP register-save-area top
- stores the prepared signed initial GP and FP offsets as 32-bit fields
- uses a reserved MIR scratch register that does not alias the destination
  `va_list` register

Focused local coverage in `backend_aarch64_machine_printer` now expects legal
`add`/`movz`/`movk`/`str` output for a selected `va_start` helper and no raw
`va.start` payload lines. Target-instruction and instruction-dispatch coverage
continue to preserve the selected-record/provenance facts, and the prepared
dump guardrails remain unchanged.

## Suggested Next

Execute Step 4 from `plan.md`: validate and classify the remaining focused
residual. The `va_start` owner has advanced `00204.c` past raw helper text; the
current first bad fact is the already-split scalar ALU immediate materialization
residual `mov w9, #503808`, owned by idea 318.

## Watchouts

- Do not reopen idea 315's large frame setup/teardown materialization owner.
- Do not repair scalar ALU immediate materialization here; that is idea 318.
- Do not fold idea 316's frame-slot/frame-layout consistency residual into
  this owner.
- Preserve prepared variadic metadata guardrails while replacing raw helper
  text in generated assembly.
- The `VaStart` lowering only initializes `va_list` fields from existing
  prepared metadata; it does not attempt scalar ALU immediate materialization,
  frame-layout repair, frame adjustment, or stack-slot memory spelling.
- `test_after.log` contains no raw `va.start` assembler diagnostics. The only
  remaining focused failure is the known `mov w9, #503808` scalar immediate
  residual.

## Proof

Focused proof command for this owner:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'
```

The focused proof was run after implementation and written to `test_after.log`.
Build succeeded; 11 tests ran; 10 passed. The local AArch64 selection/printer
tests, prepared handoff guardrails, and focused BIR/prepared dumps passed.
`c_testsuite_aarch64_backend_src_00204_c` advanced past the raw `va.start`
assembler failures and now fails only at
`build/c_testsuite_aarch64_backend/src/00204.c.s:8502` on
`mov w9, #503808`, the known scalar immediate residual owned by idea 318.
