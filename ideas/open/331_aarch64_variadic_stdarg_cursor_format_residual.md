# AArch64 Variadic Stdarg Cursor Format Residual

Status: Open
Created: 2026-05-20
Split From: ideas/open/326_aarch64_variadic_hfa_floating_residual.md

## Goal

Classify and repair the next AArch64 `00204.c` runtime blocker in the stdarg
block after the HFA/floating return residual and terminal stdarg/byval handoff
have advanced.

## Why This Exists

Idea 326 advanced the representative past the HFA/floating return-value block
and through the first stdarg/byval aggregate payload repairs. The remaining
first bad fact is no longer HFA/floating lane publication: with unbuffered
runtime output, `00204.c` reaches `stdarg:`, and the first byval payload bytes
begin as `ABCDEFGHI`, but separator bytes between the first `%9s` values are
corrupted (`0xd0`, `0xd4`, `0xd8`, ...) before a segmentation fault.

The prior lifecycle decision accepted the stdarg/byval work only as a
terminal handoff from idea 326. The next variadic cursor/format residual is a
separate owner and should be executed under this idea rather than continuing
under the HFA/floating runbook.

## In Scope

- Localize the first stdarg-block divergence after the first byval payload
  bytes are correctly sourced.
- Determine whether the owner is variadic format cursor traversal, `va_list`
  cursor progression, register-save-area or overflow-area cursor state, byval
  aggregate argument consumption order, call-boundary publication around the
  stdarg loop, or another generated-code path with direct evidence.
- Repair the classified AArch64 variadic stdarg owner generally.
- Add focused backend coverage for the repaired owner before relying on the
  external c-testsuite representative.
- Preserve the HFA/floating return, sret `x8`, byval aggregate lane, non-HFA
  aggregate `va_arg`, fixed-formal, local/value-home, frame/formal, and prior
  AArch64 variadic repairs.

## Out Of Scope

- Reopening HFA/floating argument or return lane publication unless fresh
  generated-code evidence again places the first bad fact there.
- Reopening non-HFA aggregate string `va_arg` materialization for `%7s` or
  `%9s` unless selected aggregate bytes are again missing before their
  observing calls.
- Reopening byval aggregate register-lane allocation, fragmented byval lane
  publication, sret `x8` transport, large byval indirect pointer transport,
  fixed-formal entry publication, F128 memory transport, or local/value-home
  publication without direct generated-code evidence.
- Changing semantic admission, runners, timeout policy, expectations,
  unsupported classifications, proof-log policy, or CTest registration.

## Acceptance Criteria

- The corrupted stdarg separator bytes and following segfault are localized to
  a concrete generated-code owner with evidence from generated artifacts or
  focused dumps.
- The repair is a general AArch64 variadic stdarg capability, not a named
  representative shortcut.
- Focused backend coverage proves the repaired owner and guards adjacent
  byval aggregate, HFA/floating return, non-HFA aggregate `va_arg`, call
  publication, fixed-formal, local/value-home, and frame/formal behavior.
- The focused representative either passes or advances to a new first bad fact
  that is explicitly classified for lifecycle handoff.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00204.c`, `myprintf`, one format string, one `%9s` occurrence,
  one separator byte value, one stack offset, one register, or one emitted
  instruction sequence instead of repairing a general stdarg cursor/format
  owner;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, or CTest registration to hide the failure;
- claims stdarg cursor/format progress without generated-code evidence tying
  the corrupt separator bytes or segfault to the repaired owner;
- reopens HFA/floating, non-HFA aggregate `va_arg`, byval lane publication, or
  prior local/value-home owners without direct evidence that the first bad
  fact moved back to that owner;
- adds only external c-testsuite proof without focused backend assertions for
  the repaired variadic stdarg behavior.
