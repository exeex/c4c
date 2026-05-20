Status: Active
Source Idea Path: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Stdarg Cursor/Format Residual

# Current Packet

## Just Finished

Step 1 localized the first stdarg cursor/format residual without
implementation or test edits.

The first generated-code divergence is in `myprintf`'s default character path
for `putchar(*s)`, not in the repaired `%9s` aggregate payload copy. In
`tests/c/external/c-testsuite/src/00204.c`, the first stdarg call at line 316
uses format `"%9s %9s %9s %9s %9s %9s"`. After the first `%9s` match,
`match()` leaves `s` on the `s` in the first specifier, the loop increment
advances `s` to the separator at `.str49+3`, and source line 308 should call
`putchar(*s)` with byte `0x20`.

Generated-code evidence in
`build/c_testsuite_aarch64_backend/src/00204.c.s`:

- Lines 6780..6838 copy the first `struct s9` payload and call
  `printf("%.9s", ...)`; the payload bytes are the expected `ABCDEFGHI`.
- Lines 8182..8196 show the first `stdarg()` caller passing the first two
  lanes through `x1`/`x2` and later aggregate lanes through stack overflow
  slots, so this is not the prior byval lane-publication owner.
- Lines 16021..16022 define `.str49` as `%9s %9s ...`; the first separator is
  byte `32` at `.str49+3`.
- Lines 7697..7702 lower the default `putchar(*s)` path as:
  `ldr x13, [sp]`; `ldrb w9, [x13]`; `strb w9, [sp, #1044]`; `mov w0, w13`;
  `bl putchar`.

Expected state at the first separator: `w0 == 0x20` from the loaded byte in
`w9`. Observed generated state: `w0` is copied from `w13`, the low 32 bits of
the format cursor pointer. That explains the observed separator corruption
(`0xd0`, `0xd4`, `0xd8`, ...) as pointer-low-byte output while the cursor
advances through `.str49`, and it also explains why the first payload can begin
correctly with `ABCDEFGHI` before the line diverges.

Likely owner surfaces for Step 2/3:

- `src/backend/mir/aarch64/codegen/dispatch.cpp` scalar call-argument
  publication, especially `lower_scalar_call_argument_producers`,
  `materialize_scalar_call_argument_value`,
  `materialize_call_boundary_source_to_destination`, and the
  `retarget_call_boundary_source_to_emitted_scalar` path in
  `dispatch_prepared_block`. The current generated code suggests the
  dereferenced byte producer is not authoritative for the call argument, so
  the before-call move keeps or retargets to the pointer cursor register.
- `src/backend/mir/aarch64/codegen/calls.cpp` before-call move construction
  and selected prepared register source/destination lowering for scalar GPR
  call arguments. The call argument for `putchar(*s)` must use the I8/I32
  value produced by the load, with a `w0` destination view, not the pointer
  value held in `x13`.
- If code inspection shows the wrong source value was planned before MIR,
  inspect the prealloc call-argument plan/value-home surfaces that assign the
  `putchar` argument source value. Fresh evidence currently points to scalar
  call-argument publication/source authority, not HFA/byval aggregate
  materialization.

## Suggested Next

Execute Step 2: add focused backend coverage for scalar call-argument
publication from a loaded byte value while a pointer cursor remains live, then
repair the source authority if the test exposes the same bug. A representative
dispatch fixture should avoid `00204.c`/`myprintf` names and model a simple
`sink_i32(*cursor)` or `put_byte(*cursor)` call where the generated call
argument must move the loaded byte value into `w0`, not the cursor pointer in
`xN`.

A second focused fixture, if needed, should cover the same authority through a
stored temporary byte before the call: load byte from pointer/global, store or
publish it through a local temporary, then call a one-argument scalar function.
The assertion should be semantic around source value/register view where
possible; exact emitted text is acceptable only for the final call argument
move if no structured record exposes the source choice.

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

## Proof

No build or CTest proof was required or run for this localization-only packet.
Existing executor proof remains in `test_after.log`.

Smallest proof for the next focused backend packet:
`cmake --build --preset default --target backend_aarch64_instruction_dispatch_test && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_instruction_dispatch$'`.

Acceptance proof after a repair should use the supervisor-selected subset:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|c_testsuite_aarch64_backend_src_00204_c)$'`.
