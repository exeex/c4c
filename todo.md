Status: Active
Source Idea Path: ideas/open/317_aarch64_variadic_va_start_helper_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Classify Residuals

# Current Packet

## Just Finished

Step 4 - Validate And Classify Residuals complete. The post-Step-2 first bad
fact is outside idea 317. Generated `00204.c` assembly no longer contains raw
`va.start`, `va.start.rsa`, `va.start.initial_offsets`, or `va.start.field`
helper payload lines; the `myprintf` `va_start` site now emits legal selected
assembly before the loop.

The remaining focused failure is:

- `build/c_testsuite_aarch64_backend/src/00204.c.s:8502`
- function: `subim503808`
- failing line: `mov w9, #503808`
- assembler diagnostic: `expected compatible register or logical immediate`

This is a scalar ALU immediate materialization/spelling residual, not a
variadic `va_start` helper lowering residual. It matches the already-split
owner in `ideas/open/318_aarch64_scalar_alu_immediate_materialization.md`.
There is no evidence tying the residual to `PreparedVariadicEntryHelperKind::VaStart`,
`VariadicVaStartRecord`, prepared variadic metadata, or `va_list` field
initialization.

## Suggested Next

Suggested lifecycle action: close idea 317 as complete and activate idea 318
for scalar ALU immediate materialization. Idea 317's owner has advanced the
representative past raw `va_start` helper text, and the remaining first bad
fact belongs to another already-open owner.

## Watchouts

- Do not reopen idea 315's large frame setup/teardown materialization owner.
- Do not repair scalar ALU immediate materialization here; that is idea 318.
- Do not fold idea 316's frame-slot/frame-layout consistency residual into
  this owner.
- Preserve prepared variadic metadata guardrails while replacing raw helper
  text in generated assembly.
- Do not expand idea 317 into scalar ALU immediate materialization. The
  remaining `mov w9, #503808` failure should be handed to idea 318.
- No implementation, tests, expectations, CTest registration, runners,
  unsupported classifications, timeout policy, or proof-log policy were edited
  in this classification packet.

## Proof

Focused proof command for this owner:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'
```

The focused proof was rerun for this classification packet and written to
`test_after.log`. Build succeeded; 11 tests ran; 10 passed. The local AArch64
selection/printer tests, prepared handoff guardrails, and focused BIR/prepared
dumps passed. `c_testsuite_aarch64_backend_src_00204_c` fails only at
`build/c_testsuite_aarch64_backend/src/00204.c.s:8502` on `mov w9, #503808`,
the scalar immediate residual owned by idea 318.
