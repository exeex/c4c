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
