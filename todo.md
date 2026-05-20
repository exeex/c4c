Status: Active
Source Idea Path: ideas/open/329_aarch64_variadic_aggregate_va_arg_call_setup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Post-Va Arg Call-Setup Coverage

# Current Packet

## Just Finished

Step 3 completed focused local coverage for the post-`va_arg` ordinary
call-setup repair. The existing Step 2 dispatch fixture already covered the
same owner shape: a symbol-address call argument materialized before the
retained call instruction must be republished through the later prepared
call boundary into the ABI argument register before `bl printf`.

Strengthened
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` so
`prior_stack_home_symbol_address_argument_publishes_at_later_call_boundary`
asserts both call operands in the later-boundary path: `.fmt` publishes into
`x0`, and the aggregate-member-equivalent payload operand publishes into
`x1`, with printed `adrp`/`add` materializations before `bl printf`.

This local coverage directly covers the Step 3 fixed-format-string plus later
ABI operand shape without adding a redundant `%7s`/`%9s` fixture.

## Suggested Next

Run Step 4 classification for the residual `00204.c` runtime mismatch now
that the local post-`va_arg` call-setup coverage is in place.

## Watchouts

The exact delegated proof still fails only on
`c_testsuite_aarch64_backend_src_00204_c` with a runtime output mismatch.
Do not fold that residual into the completed call-setup coverage: local
backend coverage and dump guardrails pass, and the remaining representative
failure is later broad stdarg/HFA/return/MOVI corruption rather than missing
`x0`/`x1` publication before the post-`va_arg` `printf` call.

## Proof

Ran the delegated proof command exactly:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|backend_prepare_liveness|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|c_testsuite_aarch64_backend_src_00204_c)$'`.

Result: build completed, including the updated
`backend_aarch64_instruction_dispatch_test`; 11 of 12 tests passed.
`backend_aarch64_instruction_dispatch`, `backend_aarch64_machine_printer`,
the BIR/prepared dump checks, liveness, notes, and target-instruction record
guardrails all passed. `c_testsuite_aarch64_backend_src_00204_c` failed with
the existing runtime output mismatch. Proof log: `test_after.log`.
