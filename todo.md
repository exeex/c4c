Status: Active
Source Idea Path: ideas/open/315_aarch64_large_frame_adjustment_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Classify Residuals

# Current Packet

## Just Finished

Step 4 - Validate And Classify Residuals is complete for the post-Step-2
`00204.c` first bad facts.

Classification:

- `va.start` residual: outside idea 315. The generated assembly reaches
  `build/c_testsuite_aarch64_backend/src/00204.c.s` with raw lines such as
  `va.start dest=...`, `va.start.rsa ...`, and `va.start.field ...` in
  `myprintf`, while the surrounding frame for that function is a small direct
  `sub sp, sp, #912` / `add sp, sp, #912` pair. Code evidence points to
  `src/backend/mir/aarch64/codegen/variadic.cpp::print_variadic_call`, which
  currently returns these `va.start` payload lines for
  `PreparedVariadicEntryHelperKind::VaStart`; this is variadic helper
  assembly lowering/printing, not frame setup/teardown adjustment
  materialization.
- `mov w9, #503808` residual: outside idea 315. The generated assembly failure
  is in `subim503808`, a scalar arithmetic helper from
  `tests/c/external/c-testsuite/src/00204.c` returning `x - -503808`. The
  frame setup/teardown owner is not involved; the local sequence is
  `mov w0, #0`, `mov w9, #503808`, `sub w0, w0, w9`. Code evidence points to
  scalar ALU immediate materialization in
  `src/backend/mir/aarch64/codegen/alu.cpp`, where non-plain scalar
  immediates are still emitted through `mov <scratch>, #<imm>` instead of the
  legal `materialize_integer_constant_lines` path used by frame adjustments.

## Suggested Next

Ask the plan owner to close or retire idea 315 as complete and create separate
follow-up idea(s) for the two classified residuals:

- AArch64 variadic `va_start` helper assembly lowering should replace raw
  `va.start` payload text with legal selected machine instructions.
- AArch64 scalar ALU immediate materialization should use legal constant
  materialization for non-encodable immediates such as `503808`.

## Watchouts

- Do not expand idea 315 into variadic helper lowering or scalar ALU immediate
  materialization; both are separate owners.
- Do not fold idea 316's `00216.c` frame-size/slot-offset mismatch into this
  owner.
- Do not reopen idea 314's stack-slot memory/scalar-publication paths from this
  classification.
- This packet was todo-only: no implementation files, tests, plan/source-idea
  files, expectations, unsupported classifications, runners, timeout policy,
  CTest registration, or proof-log policy were changed.

## Proof

Focused proof command for this owner:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'
```

No proof was rerun for this todo-only classification packet. Accepted baseline
evidence is `test_before.log` for this exact command: build succeeds, 11 tests
run, 10 pass, the old `frame_setup` immediate materialization diagnostic is
gone, and the only failing test is `c_testsuite_aarch64_backend_src_00204_c`
with raw `va.start` assembly lines plus illegal `mov w9, #503808`.
