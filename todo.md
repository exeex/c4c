Status: Active
Source Idea Path: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Representative And Classify Residuals

# Current Packet

## Just Finished

Step 4 - Validate Representative And Classify Residuals:
fresh proof confirms idea 328's caller-side byval aggregate lane publication is
present in generated `arg`. The regenerated `00204.c.s` now loads the prepared
`s1` byte from `[sp, #928]` into `w0` immediately before `bl fa_s1`, and packs
the two prepared `s2` bytes from `[sp, #929]`/`[sp, #930]` into `x0` with
`orr x0, x0, x9, lsl #8` immediately before `bl fa_s2`. Nearby small aggregate
calls continue the same register-lane packing pattern, while the delegated
focused backend guardrails all pass.

The remaining `00204.c` failure is no longer the Step 4 byval fixed-call
publication fault. Runtime classification with GDB localizes the segfault to
`myprintf`, where libc `printf` receives `format=0x1`. The generated `%9s`
branch after `match(&s, "%9s")` reaches `bl printf` with only `x1 = sp + 15`
set for the aggregate text buffer and without loading `.str33` (`"%.9s"`) into
`x0`; the `%7s` sibling has the same missing `.str31` publication shape. This
first bad fact is a distinct adjacent variadic aggregate `va_arg`/post-va_arg
call setup initiative, not remaining idea 328 fixed byval call-argument lane
publication and not yet the HFA/floating residual path.

## Suggested Next

Smallest next packet: split or activate a variadic aggregate `va_arg`
post-consumption call-setup packet for `myprintf`, starting with the `%7s` and
`%9s` branches that should call `printf("%.7s", t7.x)` and
`printf("%.9s", t9.x)` but currently branch with `x0 == 0x1`.

## Watchouts

Do not reopen idea 328 for this residual unless fresh evidence moves the first
bad fact back to caller-side fixed aggregate argument publication. The current
crash happens after `arg` and `ret` have been generated with the repaired byval
handoff, inside the `stdarg`/`myprintf` variadic path. The first observed
variadic failure is non-HFA `%9s`; HFA/floating cases have not become the first
bad fact yet.

## Proof

Delegated proof written to `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|backend_prepare_liveness|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build completed with `ninja: no work to do`. CTest ran 12 tests, 11
passed and 1 failed. The focused BIR/prepared-BIR and AArch64 backend guardrail
tests passed, including `backend_aarch64_instruction_dispatch`,
`backend_aarch64_machine_printer`, and the prepared 00204 AArch64 publication
dump test. The only failing test is
`c_testsuite_aarch64_backend_src_00204_c`, which exits with
`[RUNTIME_NONZERO] .../00204.c exit=Segmentation fault`.

Additional local classification command, not a replacement for the delegated
proof: `gdb -q -batch -ex 'set pagination off' -ex run -ex 'bt 8' -ex 'info registers pc x0 x1 x2 x3 x4 x5 x6 x7 sp' --args build/c_testsuite_aarch64_backend/src/00204.c.bin`
reported `__printf(format=0x1)` from `myprintf`, with `x0 = 0x1` at the
segfault.
