Status: Active
Source Idea Path: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Call-Argument Lane Coverage

# Current Packet

## Just Finished

Step 3 - Add Focused Call-Argument Lane Coverage:
`backend_aarch64_instruction_dispatch` now directly covers the prepared-source
reconstruction path for AArch64 byval aggregate register-lane publication. The
new focused case builds a BIR-backed call context with prior `StoreLocalInst`
lanes named `arg.8` and `arg.0`, maps those stores through prepared memory
accesses into one prepared frame slot, and verifies before-call lowering
reconstructs the aggregate bytes from `[sp, #96]`/`[sp, #104]` into `x0`/`x1`
before the printed `bl consume_byval`.

The existing focused size-1 direct register-source case, size-2 direct
frame-slot-source case, and generated larger stack-passed `s17` behavior remain
covered by the delegated subset.

## Suggested Next

Smallest next packet: investigate the remaining 00204 AArch64 runtime failure
in the `stdarg`/variadic call path, starting from the recursive `stdarg` frame
and the `myprintf` call that reaches libc `printf` with `format=0x1`.

## Watchouts

The Step 3 focused local coverage passed under
`backend_aarch64_instruction_dispatch`. The full delegated proof still exits
nonzero only because `c_testsuite_aarch64_backend_src_00204_c` segfaults, which
is the known residual 00204 `stdarg`/variadic runtime blocker from the prior
packet and appears outside this focused byval lane coverage slice.

## Proof

Delegated proof written to `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|backend_prepare_liveness|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build completed, rebuilding and linking
`backend_aarch64_instruction_dispatch_test`; CTest ran 12 tests, 11 passed and
1 failed. The focused BIR/prepared-BIR and AArch64 backend guardrail tests
passed, including `backend_aarch64_instruction_dispatch`. The failing test was
`c_testsuite_aarch64_backend_src_00204_c`, which exits with
`[RUNTIME_NONZERO] .../00204.c exit=Segmentation fault`.
