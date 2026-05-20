Status: Active
Source Idea Path: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Stdarg Cursor/Format Residual

# Current Packet

## Just Finished

Lifecycle switched from idea 326 to
`ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md` after the
terminal HFA/floating and stdarg/byval handoff. Idea 326 is parked with a
durable handoff note, and this active runbook now owns the new stdarg
cursor/format residual.

## Suggested Next

Execute Step 1: localize the first stdarg-block state where generated code
diverges after the first byval payload begins with `ABCDEFGHI`. Current first
bad fact: `00204.c` reaches `stdarg:`, separator bytes between the first `%9s`
values are corrupted (`0xd0`, `0xd4`, `0xd8`, ...), and execution segfaults
before completing the first stdarg line.

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

## Proof

No code validation was run by this lifecycle-only transition. Existing
executor proof remains in `test_after.log`.
