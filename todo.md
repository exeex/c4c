Status: Active
Source Idea Path: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair General Byval Aggregate Register-Lane Publication

# Current Packet

## Just Finished

Step 2 - Repair General Byval Aggregate Register-Lane Publication:
AArch64 caller-side byval aggregates of size 1 through 16 now classify as
integer-register call arguments, resolve before-call moves to the AAPCS64
argument registers, and lower publication by loading bytes/halfwords/words/
doublewords from prepared aggregate storage and packing them into the proper
low-to-high integer register lanes before `bl`.

Focused coverage was added for size-1 and size-2 register-lane publication in
`backend_aarch64_instruction_dispatch`. Generated 00204 assembly now publishes
`s1` with `ldrb w0, [sp, #928]` before `bl fa_s1`, publishes `s2` with two
byte loads packed into low `x0` before `bl fa_s2`, and publishes `s16` through
`x0`/`x1`. The larger `s17` byval aggregate remains stack-passed.

## Suggested Next

Smallest next packet: investigate the remaining 00204 AArch64 runtime failure
in the `stdarg`/variadic call path, starting from the recursive `stdarg` frame
and the `myprintf` call that reaches libc `printf` with `format=0x1`.

## Watchouts

The byval lane repair is not accepted by the full delegated proof because
`c_testsuite_aarch64_backend_src_00204_c` still segfaults. A debugger check of
`build/c_testsuite_aarch64_backend/src/00204.c.bin` places the crash in libc
`printf` parsing `format=0x1`, called from `myprintf`, with repeated `stdarg`
frames. That blocker appears outside the repaired caller-side byval aggregate
lane publication path.

## Proof

Delegated proof written to `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|backend_prepare_liveness|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build completed with `ninja: no work to do`; CTest ran 12 tests, 11
passed and 1 failed. The focused BIR/prepared-BIR and AArch64 backend guardrail
tests passed, including `backend_aarch64_instruction_dispatch`. The failing
test was `c_testsuite_aarch64_backend_src_00204_c`, which exits with
`[RUNTIME_NONZERO] .../00204.c exit=Segmentation fault`.
