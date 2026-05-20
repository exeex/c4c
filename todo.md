Status: Active
Source Idea Path: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified Stdarg Owner

# Current Packet

## Just Finished

Step 2 repaired the next stdarg byval overflow owner. Small AArch64 byval
aggregate call arguments now keep the `call_arg_byval_aggregate_register_lanes`
publication reason after the GPR argument lanes are exhausted, and MIR
call-boundary lowering has a stack-destination publication path that copies
the aggregate's prepared source lanes into outgoing stack argument slots.

Focused backend coverage was added in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` as
`overflow_byval_aggregate_call_argument_publishes_prepared_stack_lanes`. The
fixture models a neutral variadic call where the format argument uses `x0`,
three two-lane byval aggregates consume `x1`..`x6`, and the next two-lane
byval aggregate overflows to the outgoing stack area. The source aggregate has
a poison `source_stack_offset_bytes` of `7776`; the test requires stack
publication to copy from prepared source lanes at `[sp, #96]` and `[sp, #104]`
to outgoing slots `[sp, #32]` and `[sp, #40]`.

The repair is in `src/backend/prealloc/regalloc/call_moves.cpp` and
`src/backend/mir/aarch64/codegen/calls.cpp`. Prealloc now preserves the byval
lane publication reason for both register and overflow stack destinations.
AArch64 call lowering recognizes the stack-destination form, reconstructs the
prepared source bytes from the lane store facts, and emits stack-to-stack
publication through selected inline assembly instead of falling through to the
generic frame-slot stack move.

Generated-code evidence in
`build/c_testsuite_aarch64_backend/src/00204.c.s`: the first
`myprintf("%9s %9s %9s %9s %9s %9s", ...)` call no longer sources the fourth,
fifth, and sixth byval stack arguments from `[sp, #7776]`, `[sp, #7784]`, and
`[sp, #7792]`. The call setup now copies the prepared lanes:
`ldr x9, [sp, #4392]`; `str x9, [sp]`; `ldr x9, [sp, #4400]`;
`str x9, [sp, #8]`; `ldr x9, [sp, #4408]`; `str x9, [sp, #16]`;
`ldr x9, [sp, #4416]`; `str x9, [sp, #24]`; `ldr x9, [sp, #4424]`;
`str x9, [sp, #32]`; `ldr x9, [sp, #4432]`; `str x9, [sp, #40]`;
`bl myprintf`.

## Suggested Next

Continue Step 2 only if the supervisor keeps the remaining residual inside the
stdarg cursor/format owner. With unbuffered runtime output, the first stdarg
line still begins `ABCDEFGHI ABCDEFGHI ABCDEFGHI`, then the fourth `%9s`
payload prints as `\x01` and execution segfaults before completing the line.

The new first bad fact has moved into callee-side `va_arg` overflow cursor
handling for aggregate `%9s`. In `myprintf`, the `%9s` GPR-save-area path
advances `gr_offs` for the first three `struct s9` arguments. On the fourth
`%9s`, the generated overflow path should load the current `overflow_arg_area`
from `[x21]`, use it as the source address for the stack-passed aggregate, and
advance `[x21]`. Instead, at `.LBB154_25` the generated code stores the live
format/match cursor `x13` into `[x21]`, then uses `x13 + 8` as the aggregate
source. That clobbers the va_list overflow cursor and reads the fourth `%9s`
from the wrong address even though the caller stack payload is now correct.

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
instruction sequence.

## Proof

`git diff --check` passed.

Ran the delegated proof command:
`git diff --check`, then
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|c_testsuite_aarch64_backend_src_00204_c)$'`.

Current result: build succeeded; `backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, `backend_aarch64_instruction_dispatch`, and
`backend_aarch64_return_lowering` passed.
`c_testsuite_aarch64_backend_src_00204_c` still failed with `RUNTIME_NONZERO`
/ segmentation fault after the caller-side overflow byval stack publication
repair. `test_after.log` is the fresh proof log for the delegated build and
CTest command.
