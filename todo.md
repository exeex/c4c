Status: Active
Source Idea Path: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified Stdarg Owner

# Current Packet

## Just Finished

Step 2 repaired the localized stdarg cursor/format owner for scalar
call-argument publication. A loaded byte that is promoted for a scalar call
argument is now rematerialized into its prepared register from the actual byte
producer before the call-boundary move reuses that register.

Focused backend coverage was added in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` as
`scalar_byte_load_call_argument_materializes_loaded_value_not_cursor`. The
fixture models a neutral `sink_i32(*cursor)` shape: the cursor remains live in
`x13`, the byte load initially has a stack home, and the promoted scalar
argument's prepared register home also names `x13`. The test requires dispatch
to materialize the loaded byte through the pointer into the call argument path,
instead of treating the live cursor register as the scalar argument value.

The repair is in `src/backend/mir/aarch64/codegen/dispatch.cpp`. When a simple
scalar cast cannot be selected through the structured scalar-cast path but has
complete prepared register storage, dispatch now publishes the cast result
from its real same-block producer into the prepared register and records that
emitted scalar register for later call-argument retargeting. The fallback is
storage-authority guarded so existing missing-storage cast cases still fail
closed.

Generated-code evidence in
`build/c_testsuite_aarch64_backend/src/00204.c.s`: the first default
`putchar(*s)` separator path no longer lowers as `ldrb w9, [x13]` followed by
`mov w0, w13`. It now reloads through a preserved cursor register and passes
the loaded byte:
`ldr x13, [sp]`; `ldrb w9, [x13]`; `strb w9, [sp, #1044]`; `mov x9, x13`;
`ldrb w13, [x9]`; `sxtb w13, w13`; `mov w0, w13`; `bl putchar`.

## Suggested Next

Continue Step 2 only if the supervisor keeps this inside the stdarg byval
overflow argument owner. The new first bad fact has moved past the first
separator. With unbuffered runtime output, the first stdarg line now begins
`ABCDEFGHI ABCDEFGHI ABCDEFGHI`, then the fourth `%9s` payload is corrupt and
execution still segfaults before completing the line.

Generated-code evidence: for the first `myprintf("%9s %9s %9s %9s %9s %9s",
...)` call, register-passed aggregate lanes for the first three `struct s9`
arguments are loaded from `[sp, #184]`, `[sp, #4352]`, `[sp, #4360]`,
`[sp, #4368]`, `[sp, #4376]`, and `[sp, #4384]`. The fourth, fifth, and sixth
aggregate arguments should be copied to outgoing stack slots from the prepared
overflow lanes at `[sp, #4392]`/`[sp, #4400]`,
`[sp, #4408]`/`[sp, #4416]`, and `[sp, #4424]`/`[sp, #4432]`. Instead, the
generated call setup at lines 8196..8201 loads outgoing stack words from
uninitialized `[sp, #7776]`, `[sp, #7784]`, and `[sp, #7792]` before `bl
myprintf`.

The next representative focused test should cover small byval aggregate
overflow call arguments after the usable GPR argument lanes are exhausted:
three two-lane aggregates consume `x1`..`x6`, and the next two-lane aggregate
must publish its prepared source lanes to outgoing stack argument slots from
the same prepared lane sources, not from unrelated stack offsets.

## Watchouts

Do not reopen HFA/floating return publication, sret `x8`, large byval indirect
pointer transport, byval aggregate register-lane allocation, fragmented byval
lane publication, non-HFA aggregate `va_arg` materialization, fixed-formal
entry publication, local/value-home publication, or frame/formal publication
without direct generated-code evidence that the first bad fact moved back to
that owner.

Do not special-case `00204.c`, `myprintf`, one `%9s` occurrence, one separator
byte value, one stack offset, one register, one format string, or one emitted
instruction sequence.

Do not chase the `%9s` aggregate bytes for this first residual: the first
aggregate payload reaches `printf("%.9s", ...)` with the expected bytes before
the default separator path diverges.

The repaired scalar separator path should not be reopened unless generated
code again passes the cursor pointer to `putchar`. The live residual is now
the first overflow byval stack argument for the fourth `%9s` payload.

## Proof

`git diff --check` passed.

Ran the delegated proof command:
`git diff --check`, then
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|c_testsuite_aarch64_backend_src_00204_c)$'`.

Current result: build succeeded; `backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, `backend_aarch64_instruction_dispatch`, and
`backend_aarch64_return_lowering` passed.
`c_testsuite_aarch64_backend_src_00204_c` still failed with `RUNTIME_NONZERO`
/ segmentation fault. `test_after.log` is the fresh proof log for the delegated
build and CTest command.
