# AArch64 Variadic HFA And Floating Residual

Status: Closed
Created: 2026-05-19
Split From: ideas/closed/325_aarch64_variadic_local_value_home_publication.md

## Goal

Classify and repair the next AArch64 variadic `00204.c` runtime blocker where
the first HFA/floating argument output is corrupted after local/value-home
publication has been repaired.

## Why This Exists

Idea 325 repaired the local/value-home publication blockers in generated
`myprintf` and added focused local backend coverage. The representative
`c_testsuite_aarch64_backend_src_00204_c` now gets past the previously
localized publication faults but still exits with `RUNTIME_NONZERO` and a
later segmentation fault.

The first remaining bad fact is `fa_hfa11(hfa11)` printing `0.0` instead of
`11.1`, followed by corrupted floating/HFA output. The Step 2 route review for
idea 325 says this residual should not be pulled into local/value-home
publication without fresh generated-code evidence tying it back to that owner.

## In Scope

- Localize the first HFA/floating bad fact in generated AArch64 artifacts for
  `00204.c`.
- Determine whether the owner is HFA argument call-lane lowering,
  aggregate/floating `va_arg` source selection or progression, floating
  register-save-area addressing, HFA lane materialization, call-boundary
  argument transport, or another generated-code path with direct evidence.
- Repair the classified HFA/floating owner generally for AArch64 variadic or
  HFA/floating generated code.
- Add focused backend coverage for the repaired owner before relying on the
  external c-testsuite representative.
- Preserve the completed local/value-home publication repairs from idea 325
  and prior AArch64 variadic owner repairs.

## Out Of Scope

- Reopening local/value-home publication from idea 325 unless generated-code
  evidence again shows an unpublished ordinary local, constant, pattern
  operand, branch condition, call operand, or predecessor/join source.
- Reopening frame-size coverage, fixed-formal publication, `va_start`
  destination publication, aggregate helper text lowering, F128 transport,
  scalar ALU immediate materialization, large frame adjustment, or stack-slot
  memory spelling without direct generated-code evidence.
- Changing semantic admission, runners, timeout policy, expectations,
  unsupported classifications, proof-log policy, or CTest registration.
- Fixing only one named `00204.c`, one HFA shape, one float literal, one
  register lane, one stack offset, or one emitted instruction sequence.

## Acceptance Criteria

- The first HFA/floating bad fact is localized to a concrete generated-code
  owner with evidence from generated artifacts or focused dumps.
- The repair is a general AArch64 HFA/floating variadic capability, not a
  named representative shortcut.
- Focused backend coverage proves the repaired HFA/floating owner and guards
  adjacent local publication, frame/formal, `va_start`, aggregate helper,
  F128, aggregate/floating `va_arg`, scalar ALU, and frame/stack behavior.
- The focused representative either passes or advances to a new first bad fact
  that is explicitly classified for lifecycle handoff.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00204.c`, `fa_hfa11`, `hfa11`, one HFA size, one float value,
  one register, one stack offset, one `myprintf` format path, or one emitted
  instruction sequence instead of repairing a general HFA/floating owner;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, or CTest registration to hide the failure;
- claims HFA/floating progress without generated-code evidence identifying
  the source of the `0.0` versus `11.1` corruption;
- reopens idea 325's local/value-home publication or earlier AArch64 variadic
  owners without direct evidence tying the current first bad fact to that
  owner;
- adds only external c-testsuite proof without focused backend assertions for
  the repaired HFA/floating behavior.

## Lifecycle Handoff

2026-05-19: Step 4 classified the current `00204.c` segmentation fault as an
adjacent fixed-formal entry publication issue rather than a remaining
HFA/floating owner under this idea. Generated-code and LLDB evidence show the
`myprintf` fixed pointer formal `%p.format` is assigned to AArch64 storage
`x13`, while the AAPCS64 callsite passes the incoming argument in `x0`; the
callee reaches first use without publishing `x0` into the prepared home and
dereferences a null cursor.

This idea remains open and parked until the fixed-formal entry publication
owner is repaired. After that repair, resume this idea only if fresh
`00204.c` evidence again reaches an HFA/floating corruption path, variadic
floating register-save-area or overflow-area issue, HFA lane materialization
issue, or another generated-code owner within the original scope.

2026-05-20: Resumed after idea 330 closed. Fresh `00204.c` evidence now gets
past the non-HFA `%7s` / `%9s` aggregate string `va_arg` materialization
fault. Generated `myprintf` copies those selected aggregate bytes into the
destination buffers before `printf`, while the remaining runtime failure is
malformed HFA/floating output and a later segmentation fault. Current evidence
shows HFA aggregate `va_arg` payloads selected into temporary stack slots such
as `sp + 720` / `sp + 724`, but following float/double publications reading
different homes such as `sp + 556` / `sp + 560`; long-double HFA branches also
reach `.str44` calls without publishing the selected long-double values as
`printf` arguments. Resume classification from that residual, not from the
closed non-HFA string aggregate materialization route.

2026-05-20: Terminal handoff after Step 2 advanced through the HFA/floating
return-value block and the adjacent stdarg/byval aggregate repairs. The new
first bad fact is no longer HFA/floating publication: with unbuffered runtime
output, `00204.c` reaches `stdarg:`, and the first byval payload bytes begin as
`ABCDEFGHI`, but separator bytes between the first `%9s` values are corrupted
(`0xd0`, `0xd4`, `0xd8`, ...) before a segmentation fault. Active execution
split to `ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md`;
do not continue the stdarg cursor/format residual under idea 326.

2026-05-20: Reactivated after idea 331's `va_start` overflow cursor repair in
commit `77bc43d78`. The stdarg cursor first bad fact is gone, and the
representative now fails earlier in the `Arguments:` section: expected `11.1`
from `fa_hfa11(hfa11)` prints as `0.0`. Generated caller `arg` loads `hfa11`,
stores it to `[sp,#784]`, then stores stale `s8` to `[sp,#788]`, reloads that
stale home into `s13`, moves it to `s0`, and calls `fa_hfa11`; the callee
consumes `s0` normally. Resume from caller-side HFA/floating argument
publication/loading, not from stdarg cursor/format or `va_start`.

2026-05-20: Step 4 after commit `b7c41f054` classified the post-repair
residual. The HFA/floating first bad fact is gone: the representative now
prints the expected scalar/HFA lines through `34.1 34.2 34.3 34.4`. The next
visible mismatch is fixed non-HFA byval aggregate argument transport in
`fa1(struct s8, struct s9, struct s10, struct s11, struct s12, struct s13)`:
expected `stu ABC JKL TUV 456 ghi`, observed `stu ABC I JKL RS TUV`.

Generated BIR materializes bytes from `s8` through `s13` correctly and calls
`fa1(ptr byval(size=8) %t103, ptr byval(size=9) %t104, ptr byval(size=10)
%t105, ptr byval(size=11) %t106, ptr byval(size=12) %t107, ptr
byval(size=13) %t108)`. Prepared/generated evidence instead points to AAPCS64
byval aggregate argument placement across the fixed call boundary: the caller
publishes contiguous chunks through `x0` through `x6`, while callee entry
materializes `%p.a` from `x0`, `%p.b` from `x1`, `%p.c` from `x2`, `%p.d`
from `x3`, `%p.e` from `x4`, and `%p.f` from `x5`, shifting later aggregates
onto trailing chunks of earlier ones.

This idea is parked again, not closed. Source intent is satisfied enough for a
handoff, but the close-time regression guard rejected the available
`test_before.log` / `test_after.log` basis for non-increasing pass count
(`passed=3 failed=1 total=4` before and after). Continue the fixed non-HFA
byval aggregate register-lane / stack-transition residual under
`ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md`.

2026-05-20: Reactivated after idea 328's partial upper-lane byval aggregate
publication repair in commit `0a4ef64e5`. The byval `stdarg:` payloads now
preserve the target ninth bytes, and the next first bad fact is again in an
HFA/aggregate output section: expected
`33.1,33.3 34.1,34.4 34.1,34.4 34.1,34.4`, actual
`33.1,33.3 34.1,34.4 34.2,34.1 34.2,0.0`.

Resume from generated-code classification of that HFA/aggregate output
mismatch. Do not reopen byval aggregate lane publication unless fresh evidence
again shows prepared byval bytes failing to reach their AAPCS64 call lanes.

2026-05-20: Parked after commit `b5975e0cd` repaired the HFA overflow
assignment and scalar-FP symbol-load placement owners. The focused proof now
passes the prepared `00204.c` handoff dump plus byval payload helpers, and the
representative HFA long-double, double, and float sections match expected
output. The remaining `00204.c` first bad fact is later in `MOVI`: expected
`abcd0000`, actual `ffffffffabcd0000`; nearby MOVI values also show
sign-extension where expected output uses zero-extension, for example expected
`aaaaaaaa`, actual `ffffffffaaaaaaaa`.

This idea is not closed: the close-time representative is still red. The MOVI
sign-extension residual is outside this HFA/floating source scope and is
tracked separately under
`ideas/open/332_aarch64_movi_zero_extension_materialization.md`.

2026-05-21: Reactivated from the post-369 umbrella classification in
`ideas/open/295_backend_regex_failure_family_inventory.md`. The current
activating evidence is not the old HFA-output, fixed-formal, byval, stdarg
cursor, or MOVI residual. Step 2 classified a fresh AArch64 aggregate/varargs
ABI call-boundary bucket represented by `00140` and `00204`: `00140` segfaults
in calls involving a struct argument plus variadic extras, and `00204` now
fails before runtime with
`deferred_unsupported: call-boundary move node requires prepared GPR
registers, scalar FPR registers, or structured f128 q-register authority`.

Resume this idea narrowly from composite/variadic call-boundary move
preparation and publication, including the structured f128/q-register
authority path. Do not reopen prior local/value-home, fixed-formal entry,
byval lane, stdarg cursor, MOVI zero-extension, or earlier HFA-output repairs
unless fresh generated-code evidence moves the first bad fact back to those
exact boundaries.

2026-05-21: Parked after commit `1f0917f5b` reclassified the active `00204`
evidence. The apparent `HFA long double:` corruption is reachable inside the
second stdarg `%7s %9s ...` `myprintf` invocation by overreading past the
format string terminator: generated AArch64 stores the current format byte
with `strb`, then reloads an 8-byte value from the same stack address before
testing for NUL, keeping the loop alive into adjacent `.str51` / `.str52`
literals. This is a stdarg cursor/format byte materialization residual, not a
proved HFA register-save-area defect. Active execution switched back to
`ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md`; do not
continue this residual under idea 326 unless fresh evidence reaches a
standalone HFA/floating first bad fact.

2026-05-22: Parked after a Step 1 refresh found no current `00204.c` first
bad fact to localize under this source idea. The delegated proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_dump_(bir|prepared_bir).*00204|c_testsuite_aarch64_backend_src_00204_c)$'
```

passed 11/11 selected tests, including
`c_testsuite_aarch64_backend_src_00204_c`, and exposed neither an in-scope
HFA/floating or composite variadic call-boundary failure nor an adjacent owner
for handoff. Closure was considered but rejected by the close-time regression
guard because `test_before.log` and `test_after.log` both reported
`passed=11 failed=0 total=11`, so the strict monotonic check did not increase
the pass count. Keep this idea open and parked; resume only if fresh generated
evidence reaches a standalone HFA/floating, composite variadic call-boundary,
or structured f128/q-register authority first bad fact within the source
scope.

2026-05-22: Deactivated after the active Step 2 lifecycle decision for the
latest refresh. The focused proof remained green across 15 selected `00204.c`
backend dump and representative tests, and classification stayed absent: no
current in-scope HFA/floating, composite variadic call-boundary, or structured
f128/q-register authority first bad fact was available to repair or hand off.

Closure was rejected again by the strict close-time regression guard on the
matching canonical logs because both sides reported `passed=15 failed=0
total=15`; the guard failed only because the pass count did not strictly
increase. Keep this idea open and parked. Reactivate only if fresh generated
evidence reaches a standalone first bad fact inside this source scope.

2026-05-23: Closed after the Step 2 lifecycle handoff for the focused refresh
accepted the supervisor-approved non-decreasing close mode. The latest
focused `00204.c` backend dump and representative proof remained green across
15 selected tests, with no live in-scope HFA/floating, composite variadic
call-boundary, or structured f128/q-register authority first bad fact to
repair or hand off. The close-time regression guard was rerun against the
rolled-forward focused canonical log in non-decreasing mode and passed with
`passed=15 failed=0 total=15` before and after, `delta=0`, and no new failing
tests. The source idea is therefore complete rather than merely parked.
