Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove The AArch64 Targeted Cases

# Current Packet

## Just Finished

Completed Step 5 of `plan.md`: reran the targeted AArch64 proof scope after the
global byval source-selection repair and characterized the remaining
`00204.c` runtime mismatch.

Focused result:

- `backend_prepare_frame_stack_call_contract` passes.
- Guard cases `c_testsuite_aarch64_backend_src_00032_c` and
  `c_testsuite_aarch64_backend_src_00182_c` pass.
- `c_testsuite_aarch64_backend_src_00204_c` still fails with
  `[RUNTIME_MISMATCH]`.

Remaining `00204.c` output signature:

- The scalar MOVI/OPI tail still matches expected output.
- The first bad family is already in direct aggregate arguments, before
  `stdarg`: the `Arguments:` section prints repeated control/count bytes
  instead of the global struct strings, and direct HFA arguments print mostly
  `0.0`.
- Direct aggregate returns are also corrupt before `stdarg`: the `Return
  values:` struct section prints repeated `!` bytes and direct HFA returns are
  mostly `0.0`.
- The `stdarg` string/integer and HFA-vararg sections remain bad, but they are
  downstream of unresolved direct aggregate argument/return corruption and
  should not be treated as the next isolated root boundary yet.

Post-repair artifacts:

- `--dump-bir --target aarch64-linux-gnu --mir-focus-function arg` still shows
  expected `.global.aggregate.load.` lanes feeding the byval temporaries before
  calls such as `fa_s1`.
- `--dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function arg`
  records the byval callsites as aggregate carriers/preservation sources, for
  example `fa_s1` has `arg0 bank=gpr from=register:x21 to=x0` for `%t2` rather
  than an emitted payload lane load.
- The emitted AArch64 assembly for `arg` materializes early byval arguments
  from stale call-result state, e.g. after `printf("Arguments:\n")` it stores
  `w13` to a stack byte and passes that through `w0` to `fa_s1`, which explains
  the repeated control/count-byte output.

## Suggested Next

Trace the next ABI boundary at prepared-call to AArch64 machine-materialization
for direct aggregate byval arguments and direct memory returns. The immediate
question is why `.global.aggregate.load.` payload lanes present in semantic BIR
are not materialized into the call argument registers or sret destination bytes
in the final AArch64 assembly.

## Watchouts

- Do not downgrade `00204.c` expectations, mark it unsupported, or special-case
  its literal output shape.
- Keep `00032.c` and `00182.c` visible as guard cases.
- Do not start with stdarg/HFA-vararg repair: the current signature proves
  direct aggregate byval arguments and direct aggregate/sret returns are already
  wrong before the variadic sections execute.
- The likely next owned boundary is AArch64 call-result/preservation/source
  materialization for byval register lanes and sret memory returns after
  prepared BIR, not source selection in semantic BIR.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00032_c|c_testsuite_aarch64_backend_src_00182_c)$'
```

Result: exit code 8. The focused contract and both guard cases passed;
`c_testsuite_aarch64_backend_src_00204_c` remains the only failing test in this
scope with a runtime mismatch. Canonical executor proof log:
`test_after.log`.
