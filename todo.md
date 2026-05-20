Status: Active
Source Idea Path: ideas/open/330_aarch64_non_hfa_aggregate_va_arg_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Representative And Classify Residuals

# Current Packet

## Just Finished

Step 4 ran the supervisor-selected representative proof and classified the
remaining `00204.c` residual. The focused backend guardrails pass, and the
external representative still fails only after the repaired non-HFA string
aggregate `va_arg` cases have printed their expected bytes.

Generated `myprintf` now copies `%7s` bytes from the selected `va_arg` source
through byte loads/stores into the destination buffer at `sp + 8` before
`adrp/add x0, .str31`, `add x1, sp, #8`, and `bl printf`. The `%9s` branch
likewise copies nine bytes into `sp + 15` before `adrp/add x0, .str33`,
`add x1, sp, #15`, and `bl printf`. This confirms the idea 330 non-HFA
aggregate materialization fault is gone for the representative string cases
and that idea 329 call publication remains repaired for those calls.

The remaining first bad fact belongs to the later variadic HFA/floating
residual path, not to non-HFA string aggregate materialization. The fresh
runtime output reaches the HFA/long-double region, prints malformed numeric
values, and eventually segfaults. In the generated `myprintf` HFA branches,
aggregate `va_arg` payloads are selected into temporary stack slots such as
`sp + 720` / `sp + 724`, but the following float/double publications read
different homes such as `sp + 556` / `sp + 560`. The long-double HFA branches
select stack payload slots and then call `printf` with `.str44` without
publishing the selected long-double values as `printf` arguments. This is a
lifecycle handoff candidate for the existing HFA/floating residual owner
surface from idea 326 or a successor focused on variadic HFA materialization
and long-double call-argument publication.

## Suggested Next

Hand the representative residual back to lifecycle ownership for the
variadic HFA/floating path. The smallest next packet should localize the first
HFA/long-double `myprintf` branch that publishes values from homes different
from the selected `va_arg` payload, then decide whether the existing idea 326
owner is sufficient or whether a new focused successor idea is needed.

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

Do not reopen the non-HFA `%7s` / `%9s` materialization route based on the
current representative failure. The string aggregate bytes are copied into the
destination buffers before `printf`, and the remaining crash is downstream in
HFA/floating or long-double handling.

There is also a literal-character fallback smell in `myprintf`: the generated
`putchar` path loads the current byte into `w9` but moves `w13` into `w0`.
That can garble label-only `myprintf` calls, but the malformed numeric HFA
output and missing long-double argument publication are still the representative
residual to classify for lifecycle handoff.

The delegated proof command pipes CTest through `tee`; inspect `test_after.log`
instead of relying only on the shell status because the pipeline masks the
remaining failing CTest result.

## Proof

Ran delegated Step 4 proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|c_testsuite_aarch64_backend_src_00204_c)$' | tee test_after.log`.
Build succeeded. Result: 5/6 selected tests passed:
`backend_aarch64_prepared_memory_operand_records`,
`backend_aarch64_memory_operand_contract`,
`backend_aarch64_instruction_dispatch`,
`backend_aarch64_machine_printer`, and
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
passed. `c_testsuite_aarch64_backend_src_00204_c` still fails with
`RUNTIME_NONZERO` / segmentation fault after malformed HFA/floating output.
The post-repair crash moved past the non-HFA string aggregate `va_arg`
source-pointer copies and is now classified to later variadic HFA/floating or
long-double handling; proof log path: `test_after.log`.
