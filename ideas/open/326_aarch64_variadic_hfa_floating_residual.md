# AArch64 Variadic HFA And Floating Residual

Status: Open
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
