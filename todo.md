Status: Active
Source Idea Path: ideas/open/330_aarch64_non_hfa_aggregate_va_arg_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Non-HFA Aggregate Va Arg Materialization Coverage

# Current Packet

## Just Finished

Step 3 added focused AArch64 dispatch coverage for the repaired non-HFA
aggregate `va_arg` byte-copy materialization path. The new contract builds a
small prepared block that computes a source pointer, performs prepared
pointer-value byte loads into stack-slot result homes, publishes those loaded
bytes back to the destination frame slot, then emits the observing `printf`
call. The assertion checks the ordered route:
`add x20, x21, #8`, `ldrb`/`strb` byte publications at `sp + 64` and
`sp + 65`, call-argument publication, then `bl printf`.

This coverage is local to the repaired non-HFA string-aggregate copy owner and
does not weaken runtime expectations or special-case `00204.c`, `%7s`, `%9s`,
`myprintf`, one branch, or one source register sequence.

## Suggested Next

Localize the downstream `00204.c` failure in the later variadic aggregate/HFA
or long-double handling after the string cases. Start from the first malformed
floating/long-double output near the later `printf` format strings in
`myprintf`, then compare the prepared `va_arg` producer publication and emitted
loads/stores for those aggregate cases.

## Watchouts

Avoid replaying all prepared memory accesses at the call boundary. The focused
dispatch coverage now pins ordinary-block prepared pointer-value byte loads
with stack-slot result publication before the observing call.

Preserve the source idea guardrails: do not special-case `00204.c`,
`myprintf`, `%7s`, `%9s`, `x13`, `sp + 8`, `sp + 15`, one aggregate size, one
branch, or one emitted copy sequence. Do not weaken expectations,
unsupported classifications, runner behavior, timeout policy, CTest
registration, or proof-log behavior.

Do not record stack-publication scratch registers as stable scalar homes; those
registers are only temporary carriers for immediate stack publication and may be
reused by later ordinary memory operations.

Do not treat the current `x21` `va_list` field repair as sufficient by itself:
the representative is still red, but the remaining crash is now after the
non-HFA source-pointer producer order has been repaired.

The delegated proof command pipes CTest through `tee`; inspect `test_after.log`
instead of relying only on the shell status because the pipeline masks the
remaining failing CTest result.

## Proof

Ran delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|c_testsuite_aarch64_backend_src_00204_c)$' | tee test_after.log`.
Build succeeded. Result: 5/6 selected tests passed:
`backend_aarch64_prepared_memory_operand_records`,
`backend_aarch64_memory_operand_contract`,
`backend_aarch64_instruction_dispatch`,
`backend_aarch64_machine_printer`, and
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
passed. `c_testsuite_aarch64_backend_src_00204_c` still fails with
`RUNTIME_NONZERO` / segmentation fault. The post-repair crash moved past the
non-HFA string aggregate `va_arg` source-pointer copies and is now localized to
later variadic aggregate/HFA or long-double handling; proof log path:
`test_after.log`.
