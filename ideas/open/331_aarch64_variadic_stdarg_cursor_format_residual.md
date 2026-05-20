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

## Lifecycle Handoff

2026-05-20: Commit `77bc43d78` repaired the AArch64 `va_start` overflow
cursor owner by initializing `overflow_arg_area` above the actual fixed callee
frame and named incoming stack formal area. The old stdarg cursor first bad
fact is gone: generated `myprintf` initializes the va_list overflow cursor
from `sp + 1344`, and the prior stack `%9s` path continues to reload and
advance the cursor through `[x21]`.

The representative still fails, but its first bad fact moved earlier than
`stdarg`: in the `Arguments:` section, `fa_hfa11(hfa11)` should print `11.1`
but prints `0.0`; generated caller `arg` stores loaded `hfa11` at
`[sp,#784]` while passing the stale `s8` path through `[sp,#788]`, and callee
`fa_hfa11` consumes `s0` normally. That residual belongs to caller
HFA/floating argument publication under
`ideas/open/326_aarch64_variadic_hfa_floating_residual.md`, not to this stdarg
cursor/format idea.

Close was not accepted in this lifecycle turn because the available
`test_before.log` and `test_after.log` are identical and the close-time
regression guard rejected them for non-increasing pass count. Keep this idea
open but inactive until a supervisor accepts closure with an appropriate
regression-guard basis.

2026-05-20: Reactivated after idea 328's fixed non-HFA byval placement repair
in commit `941d0c1cb`. The byval `Arguments:` and `Return values:` sections
now match through the repaired fixed aggregate case, so the old byval
caller/callee rounded-slot fault is no longer the representative blocker.

The current first bad fact is again in the later `stdarg:` block. The first
visible actual line is `ABCDEFGHI ABCDEFGHI ABCDEFGHI stdarg:` where the
expected output has six `ABCDEFGHI` fields before `stdarg:`. Broad supervisor
validation before the handoff commit passed
`ctest --test-dir build -j --output-on-failure -R '^backend_'` at 100%.
Resume from stdarg field production, cursor progression, or argument
publication evidence; do not reopen fixed byval placement without fresh
generated-code evidence.

2026-05-20: Step 4 classified the advanced stdarg payload residual as outside
this stdarg cursor/format idea. The first `stdarg:` payload line is fixed and
prints six `ABCDEFGHI` fields. The next first bad fact is the second payload
line, expected `lmnopqr ABCDEFGHI ...` but observed `lmnopqr ABCDEFGH ...`.
Generated evidence shows the ninth byte exists before call publication, then
is stored to separate temporary stack slots while the high byval lane is loaded
from unpopulated aggregate lane slots. `myprintf` cursor progression, aggregate
`va_arg` copy count, format traversal, and destination buffering are downstream
observers. Handoff back to
`ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md` for
partial upper-lane byval aggregate publication.
