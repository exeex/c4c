Status: Active
Source Idea Path: ideas/open/330_aarch64_non_hfa_aggregate_va_arg_materialization.md
Source Plan Path: plan.md
Current Step ID: 2.3
Current Step Title: Repair Remaining Non-HFA Aggregate Va Arg Copy Semantics If Needed

# Current Packet

## Just Finished

Step 2.3 repaired the expanded non-HFA aggregate `va_arg` source-pointer
producer materialization/order observed after the Step 2.2 byte-copy
publication repair. AArch64 dispatch now replays predecessor-terminator
out-of-SSA select-materialization moves for stack-home sources, reloads
prepared `va_list` field values from the live published base register during
edge publication, lowers edge `add`/`sub` producers recursively, and suppresses
stale block-entry stack-source moves that belong to predecessor-edge parallel
copies.

Generated `build/c_testsuite_aarch64_backend/src/00204.c.s` no longer lowers
the `%t20`/`%t23`/`%t48` style non-HFA source-pointer copies from stale stack
homes before their producers. The `vaarg.join.14` register path now
materializes the source pointer before the byte loads that feed `printf("%.7s")`,
and the `vaarg.join.39` overflow path computes the `%t23` source before the
byte loads that feed `printf("%.9s")`. The representative program now prints
through the string aggregate cases that previously exposed the unmaterialized
source-pointer crash.

The selected representative still fails later. The new first bad fact is
downstream of the repaired non-HFA string aggregate source-pointer order: after
printing all string cases and several floating aggregate cases, output becomes
malformed in the later variadic aggregate/HFA or long-double region and the
program segfaults. This is no longer the `%t20`/`%t23`/`%t48` non-HFA
source-pointer-before-copy failure owned by this packet.

## Suggested Next

Localize the downstream `00204.c` failure in the later variadic aggregate/HFA
or long-double handling after the string cases. Start from the first malformed
floating/long-double output near the later `printf` format strings in
`myprintf`, then compare the prepared `va_arg` producer publication and emitted
loads/stores for those aggregate cases.

## Watchouts

Avoid replaying all prepared memory accesses at the call boundary. The missing
records are already in the ordinary block before the call, and this packet now
emits the pointer byte loads plus frame-slot copies before the observing
`printf` calls.

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
instead of relying only on the shell status because the pipeline can mask a
failing CTest result.

## Proof

Ran delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_aarch64_instruction_dispatch|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|c_testsuite_aarch64_backend_src_00204_c)$' | tee test_after.log`.
Build succeeded. Result: 4/5 selected tests passed:
`backend_aarch64_prepared_memory_operand_records`,
`backend_aarch64_memory_operand_contract`,
`backend_aarch64_instruction_dispatch`, and
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
passed. `c_testsuite_aarch64_backend_src_00204_c` still fails with
`RUNTIME_NONZERO` / segmentation fault. The post-repair crash moved past the
non-HFA string aggregate `va_arg` source-pointer copies and is now localized to
later variadic aggregate/HFA or long-double handling; proof log path:
`test_after.log`.
