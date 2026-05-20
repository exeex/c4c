Status: Active
Source Idea Path: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified Stdarg Owner

# Current Packet

## Just Finished

Step 2 repaired the callee-side classified stdarg owner for repeated AArch64
small-aggregate overflow cursor loads. BIR lowering no longer lets a tracked
aggregate-field pointer alias replace a fresh load from mutable AArch64
`va_list` storage, so repeated stack `va_arg` paths observe the current
`overflow_arg_area` value.

Focused backend coverage was added in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` as
`lir_to_bir_reloads_mutated_aarch64_tracked_pointer_field`. The fixture lowers
an AArch64 LIR function that stores a pointer into a struct field, loads it,
stores an advanced pointer back to the same field, then loads the field again.
It requires the second read to be a `bir.load_local ptr %lv.box.0` and the
follow-on pointer add to use `%second`, not the stale `%advanced` alias.

Generated-code evidence in
`build/c_testsuite_aarch64_backend/src/00204.c.s`: the old fourth `%9s`
overflow block at `.LBB154_25` no longer stores the live format/match cursor
`x13` into `[x21]`. It now reloads the current overflow cursor with
`ldr x13, [x21]`, advances it with `add x9, x13, #16`, and stores the advanced
cursor back to `[x21]` before using the loaded `x13` as the aggregate source.

## Suggested Next

Continue Step 2 on the remaining AArch64 `va_start`/entry-side overflow
cursor initialization. The old `.LBB154_25` clobber is fixed, but
`c_testsuite_aarch64_backend_src_00204_c` still fails at runtime.

New first bad fact: in `myprintf`, the callee prologue allocates
`sub sp, sp, #1344`, then initializes the `va_list` overflow cursor with
`add x9, sp, #1264; str x9, [x21]` while `x21` is the local va_list base
`sp + 1264`. That makes `overflow_arg_area` point at the va_list/local save
area instead of the incoming caller overflow argument area above the callee
frame. The fourth `%9s` stack path now correctly loads `[x21]` and advances
it, but it is advancing a cursor that was initialized to the wrong address.

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

Ran the delegated proof command:
`git diff --check`, then
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|c_testsuite_aarch64_backend_src_00204_c)$'`.

Current result: build succeeded; `backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, `backend_aarch64_instruction_dispatch`, and
`backend_aarch64_return_lowering` passed.
The passing `backend_aarch64_instruction_dispatch` run includes
`lir_to_bir_reloads_mutated_aarch64_tracked_pointer_field`, the focused
coverage for the AArch64 tracked pointer-field reload rule.
`c_testsuite_aarch64_backend_src_00204_c` still failed with
`RUNTIME_MISMATCH`; the old `.LBB154_25` cursor-clobber first bad fact is no
longer present, and the new first bad fact is the incorrect initial
`overflow_arg_area` address above. `test_after.log` is the fresh proof log for
the delegated build and CTest command.
