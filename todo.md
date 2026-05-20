Status: Active
Source Idea Path: ideas/open/329_aarch64_variadic_aggregate_va_arg_call_setup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair General Post-Va Arg Ordinary Call Operand Publication

# Current Packet

## Just Finished

Step 2 repaired the general AArch64 prepared callsite publication path for
symbol-address arguments whose prepared address materialization is recorded
before the retained call instruction. `lower_address_materializations` now
bridges prior same-block call-argument symbol materializations through the
current `PreparedCallPlan` into the ABI destination register, and
`lower_before_call_move` suppresses the redundant stack-home symbol move when
that prior materialization is available.

Focused coverage was added in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` for a
stack-home symbol-address call argument with a prior address materialization
that must publish directly into `x0` at the later call boundary before
`bl printf`.

Generated AArch64 for `tests/c/external/c-testsuite/src/00204.c` now emits
`adrp x0, .str31; add x0, x0, :lo12:.str31; add x1, sp, #8; bl printf` for
the `%7s` branch and
`adrp x0, .str33; add x0, x0, :lo12:.str33; add x1, sp, #15; bl printf` for
the `%9s` branch.

## Suggested Next

Classify the newly exposed `00204.c` runtime mismatch after the fixed
format-string publication. The representative now gets past the prior libc
`printf` crash, but the external output still diverges in later stdarg/HFA
and return-value surfaces.

## Watchouts

Do not treat the remaining external mismatch as the old format-string
publication gap: generated code now publishes `.str31`/`.str33` into `x0`
before `bl printf` and preserves the aggregate buffer address in `x1`. The
proof still includes broad residual corruption in `00204.c`; that should be
classified as the next first bad fact rather than folded into this completed
symbol-address publication repair.

## Proof

Ran the delegated proof command exactly:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|backend_prepare_liveness|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|c_testsuite_aarch64_backend_src_00204_c)$'`.

Result: build completed with `ninja: no work to do`; 11 of 12 tests passed.
`c_testsuite_aarch64_backend_src_00204_c` now fails with a runtime output
mismatch rather than the prior `Segmentation fault`. Proof log:
`test_after.log`.
