Status: Active
Source Idea Path: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified Stdarg Owner

# Current Packet

## Just Finished

Step 2 repaired AArch64 `va_start` overflow cursor initialization. The
structured `VariadicVaStartRecord` now carries a lowering-time override for
`overflow_arg_area` based on the actual fixed callee frame adjustment plus the
named fixed-parameter incoming stack area, instead of reusing the zero-sized
local overflow-base frame slot offset.

Focused backend coverage was added in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` as
`variadic_va_start_initializes_overflow_cursor_above_callee_frame`. The fixture
uses a variadic entry with a stack-passed fixed formal and verifies the
computed `overflow_arg_area` offset is after both the callee frame and named
incoming stack formal area, not the prepared local overflow-base slot.

Generated-code evidence in
`build/c_testsuite_aarch64_backend/src/00204.c.s`: `myprintf` still allocates
`sub sp, sp, #1344`, but now initializes the va_list overflow cursor with
`add x9, sp, #1344; str x9, [x21]`. The old local va_list/save-area
initialization (`add x9, sp, #1264; str x9, [x21]`) is gone. The prior
`.LBB154_25` fix also remains intact: that stack `%9s` path reloads
`ldr x13, [x21]`, advances with `add x9, x13, #16`, and stores the advanced
cursor back to `[x21]`.

## Suggested Next

Continue Step 2 by deciding whether the next packet should stay on the
remaining `00204.c` runtime mismatch or split to the earlier HFA argument
publication route.

New first bad fact after the va_start repair: `c_testsuite_aarch64_backend_src_00204_c`
still fails, but the first runtime mismatch is now earlier than `stdarg`.
In the `Arguments:` section, expected `11.1` from `fa_hfa11(hfa11)` prints as
`0.0`. Generated code in `arg` loads `hfa11` into `s13` and stores it at
`[sp, #784]`, then stores stale `s8` at `[sp, #788]`, reloads `[sp, #788]`
into `s13`, moves that into `s0`, and calls `fa_hfa11`. The callee
`fa_hfa11` consumes `s0` normally, so the first bad fact appears to be caller
HFA float argument publication/loading, not the repaired `va_start`
overflow cursor.

## Watchouts

Do not reopen HFA/floating return publication, sret `x8`, large byval indirect
pointer transport, byval aggregate register-lane allocation, fragmented byval
lane publication, non-HFA aggregate `va_arg` materialization, fixed-formal
entry publication, local/value-home publication, frame/formal publication, the
scalar separator call-argument repair, or the byval overflow stack publication
repair without direct generated-code evidence that the first bad fact moved
back to that owner.

Do not special-case `00204.c`, `myprintf`, one `%9s` occurrence, one separator
byte value, one stack offset, one register, one format string, or one emitted
instruction sequence. The va_start repair computes the cursor from general
callee-frame and named-stack-formal facts; keep that route semantic if it
needs further adjustment.

## Proof

Ran the delegated proof command:
`git diff --check`, then
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|c_testsuite_aarch64_backend_src_00204_c)$'`.

Current result: `git diff --check` passed and the build succeeded.
`backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, `backend_aarch64_instruction_dispatch`, and
`backend_aarch64_return_lowering` passed. The passing
`backend_aarch64_instruction_dispatch` run includes both
`lir_to_bir_reloads_mutated_aarch64_tracked_pointer_field` and
`variadic_va_start_initializes_overflow_cursor_above_callee_frame`.
`c_testsuite_aarch64_backend_src_00204_c` still failed with
`RUNTIME_MISMATCH`; the old `.LBB154_25` cursor-clobber and the later
local-va_list overflow cursor initialization first bad facts are no longer
present. `test_after.log` is the fresh proof log for the delegated build and
CTest command.
