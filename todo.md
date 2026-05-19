Status: Active
Source Idea Path: ideas/open/325_aarch64_variadic_local_value_home_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Local Value Home Publication Fault

# Current Packet

## Just Finished

Step 1 localized the current `00204.c` AArch64 variadic local/value-home
publication fault. In generated `myprintf`, the first bad fact is:
after the prologue stores `%p.format` at `[sp, #624]` and initializes the
`va_list` record, control branches to `.LBB154_12` and immediately emits
`cmp w13, #0` before any same-function definition of `w13`. The next visible
bad home is the first `match` call loading `x1` from `[sp, #640]` before that
slot is published.

Prepared tracing maps the bad facts as follows:
`for.cond.70` has `%t2 = bir.ne i8 %t1, 0` feeding `bir.cond_br i32 %t2`;
the prepared storage plan assigns `%t2` to a frame slot at `stack+976`, but
the emitted branch path prints a register test against `w13` without loading
or otherwise publishing `%t2`. `block_70` calls
`match(ptr %lv.s, ptr %t5)`; the prepared call plan says `arg1` comes from
`frame_slot:stack+640` and the storage plan maps `%t5` to
`slot#2946+stack640`, but the generated call path emits
`ldr x1, [sp, #640]` before any store to that slot. The first printf path
has the same shape for `%t27` at `stack+648` and `%lv.t7.0` at `stack+656`.

Likely owning surfaces for Step 2:
`src/backend/mir/aarch64/codegen/dispatch.cpp`
(`emit_value_publication_to_register`,
`materialize_missing_frame_slot_call_arguments`, and the conditional-branch
dispatch path), `src/backend/mir/aarch64/codegen/comparison.cpp`
(`lower_prepared_conditional_branch_terminator` / compare operand installation),
`src/backend/mir/aarch64/codegen/machine_printer.cpp`
(`print_branch`, `print_fused_compare_branch`, and compare operand spelling),
and `src/backend/mir/aarch64/codegen/operands.cpp` (`resolve_value_operand`
turning stack homes into printable operands). The prepared facts themselves
look present; the missing rule is publication/materialization of stack-home
values before branch tests, matcher operands, and printf operands consume
them.

## Suggested Next

Execute Step 2 from `plan.md`: repair the AArch64 prepared-value-home
publication rule generally, starting with materializing stack-slot branch
conditions/compare operands and frame-slot call arguments before use rather
than relying on an uninitialized scratch register or an unpublished spill
slot. Keep the repair semantic; do not special-case `00204.c`, `myprintf`,
`%t2`, `%t5`, `stack+640`, or `w13`.

Smallest focused proof command for the repair:
`cmake --build --preset default && bash -o pipefail -c "ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_(machine_printer|instruction_dispatch|target_instruction_records|prepared_branch_records|branch_compare_records)|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|c_testsuite_aarch64_backend_src_00204_c)$' | tee test_after.log"`

## Watchouts

- Preserve prior repairs: large stack offsets, large frame adjustments,
  `va_start` helper lowering, scalar ALU immediates, HFA argument lanes, F128
  transport, aggregate helper text lowering, `va_start` destination
  materialization, aggregate/floating `va_arg` source/progression, frame-size
  coverage, and fixed-formal publication.
- Representative local tests for the repair should include the existing
  AArch64 branch, instruction-dispatch, target-record, machine-printer, and
  prepared-BIR surfaces named in the suggested proof, plus the external
  `c_testsuite_aarch64_backend_src_00204_c` representative.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, the format loop, one
  local, one stack slot, one register, one offset, or one emitted instruction
  sequence.
- Do not weaken expectations, unsupported classifications, CTest registration,
  runner behavior, timeout policy, proof-log policy, or prior guardrails.
- Reopen frame/formal publication only if fresh generated output again shows
  uncovered stack references or fixed-formal clobber before publication.

## Proof

Delegated proof ran:
`cmake --build --preset default && bash -o pipefail -c "ctest --test-dir build -j --output-on-failure -R '^(backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|c_testsuite_aarch64_backend_src_00204_c)$' | tee test_after.log"`

Result: build was up to date; 5/6 tests passed. The focused BIR/prepared
handoff tests passed, and `c_testsuite_aarch64_backend_src_00204_c` failed
with `RUNTIME_NONZERO`, `exit=Segmentation fault`. Proof log:
`test_after.log`.
