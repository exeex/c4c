Status: Active
Source Idea Path: ideas/open/317_aarch64_variadic_va_start_helper_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Va Start Helper Payloads

# Current Packet

## Just Finished

Step 1 - Localize Va Start Helper Payloads complete. The missing fact is not
upstream variadic metadata: `00204.c` reaches selected AArch64 printing with a
structured `VariadicVaStartRecord`, but
`src/backend/mir/aarch64/codegen/variadic.cpp::print_variadic_call` prints that
record as raw helper payload text (`va.start`, `va.start.rsa`,
`va.start.initial_offsets`, and `va.start.field`) instead of legal AArch64
stores.

The representative `myprintf` helper payload is:

- destination `va_list`: `%lv.ap` / value#2051 in register `x13`
- register save area: slot #2569, stack+672, size 192, align 16, GP base 0, FP
  base 64, GP slot 8, FP slot 16, saved GP 7, saved FP 8
- initial offsets: GP -56, FP -128
- overflow base: slot #2570, stack+864, align 8
- AAPCS64 `va_list` fields: `overflow_arg_area@0:8`,
  `gp_register_save_area@8:8`, `fp_register_save_area@16:8`,
  `gp_offset@24:4`, `fp_offset@28:4`
- helper resources: one scratch register, zero scratch stack

Owning code surface for the repair:

- `src/backend/mir/aarch64/codegen/variadic.cpp::print_variadic_call`, in the
  `PreparedVariadicEntryHelperKind::VaStart` branch that currently builds raw
  text lines from `VariadicVaStartRecord`
- `src/backend/mir/aarch64/codegen/variadic.cpp::make_variadic_va_start_record`
  and `src/backend/mir/aarch64/codegen/instruction.hpp::VariadicVaStartRecord`
  are already the selected-record carrier and should be preserved as the
  structured source facts
- `src/backend/prealloc/variadic_entry_plans.cpp` is the upstream AAPCS64
  metadata producer and is not the current missing-lowering owner

## Suggested Next

Execute Step 2 from `plan.md`: replace the `VaStart` raw text printer branch
with legal AArch64 selected output that initializes the destination `va_list`.
The repair should materialize stack-relative addresses for the overflow area,
GP register-save area, and FP register-save area, then store those pointers and
the signed GP/FP offsets into the field offsets provided by
`VariadicVaStartRecord::va_list_fields`.

## Watchouts

- Do not reopen idea 315's large frame setup/teardown materialization owner.
- Do not repair scalar ALU immediate materialization here; that is idea 318.
- Do not fold idea 316's frame-slot/frame-layout consistency residual into
  this owner.
- Preserve prepared variadic metadata guardrails while replacing raw helper
  text in generated assembly.
- Existing local coverage currently proves selection/printing of the raw
  helper records: `backend_aarch64_target_instruction_records`,
  `backend_aarch64_instruction_dispatch`, and
  `backend_aarch64_machine_printer`. Step 2 should update or add
  `backend_aarch64_machine_printer` coverage so `va_start` output is legal
  assembly and contains no raw `va.start` payload lines, without weakening the
  prepared handoff/dump tests.
- `test_before.log` shows the focused baseline still has 10/11 passing tests:
  only `c_testsuite_aarch64_backend_src_00204_c` fails, first on raw
  `va.start` lines and later on the scalar immediate residual
  `mov w9, #503808` owned by idea 318.

## Proof

Focused proof command for this owner:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'
```

This packet is localization-only and owns no implementation files. The same
focused proof was run after the `todo.md` update and written to
`test_after.log`; build succeeded, 11 tests ran, 10 passed, and only
`c_testsuite_aarch64_backend_src_00204_c` failed. The failure matches the
current baseline: first raw `va.start` helper lines at
`build/c_testsuite_aarch64_backend/src/00204.c.s:4611-4618`, then the later
known scalar immediate `mov w9, #503808` residual owned by idea 318. This proof
remains sufficient for the current baseline because it includes build proof,
the local AArch64 selection/printer tests, prepared handoff guardrails, focused
BIR/prepared dumps for `00204.c`, and the external `00204.c` representative.
