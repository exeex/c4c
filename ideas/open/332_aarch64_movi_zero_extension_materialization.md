# AArch64 MOVI Zero-Extension Materialization

Status: Open
Created: 2026-05-20
Split From: ideas/open/326_aarch64_variadic_hfa_floating_residual.md

## Goal

Classify and repair the AArch64 MOVI integer materialization residual where
`00204.c` now prints sign-extended values in a block whose expected output
uses zero-extended payloads.

## Why This Exists

Idea 326 repaired the HFA overflow assignment and scalar-FP symbol-load
placement owners in commit `b5975e0cd`. The representative now gets past the
HFA long-double, double, and float output sections, while the next first bad
fact is later in `MOVI`: expected `abcd0000`, actual
`ffffffffabcd0000`. Nearby MOVI values also show the same pattern, for example
expected `aaaaaaaa`, actual `ffffffffaaaaaaaa`.

The residual is not HFA/floating publication, byval aggregate lane
publication, stdarg cursor handling, or fixed-formal entry publication. It
needs a focused integer immediate or move-materialization route.

## In Scope

- Localize the generated-code owner for MOVI values that are sign-extended
  where C output expects zero-extension.
- Determine whether the fault is immediate construction, source integer width
  tracking, extension opcode choice, register width selection, or call/output
  argument materialization.
- Repair the classified AArch64 integer materialization owner generally.
- Add focused backend coverage that proves the MOVI zero-extension behavior
  without relying only on the external `00204.c` representative.
- Preserve completed HFA overflow assignment, HFA payload materialization,
  byval aggregate lane publication, stdarg cursor, fixed-formal entry, and
  local/value-home publication repairs.

## Out Of Scope

- Reopening HFA/floating argument, return, overflow, or symbol-load placement
  repairs unless fresh generated-code evidence again shows HFA corruption.
- Reopening byval aggregate lane publication, aggregate `va_arg`, stdarg
  cursor progression, fixed-formal entry publication, local/value-home
  publication, frame/formal publication, or large frame layout without direct
  generated-code evidence.
- Changing semantic admission, runners, timeout policy, expectations,
  unsupported classifications, proof-log policy, or CTest registration.
- Fixing only `00204.c`, one MOVI literal, one output line, one register, one
  stack slot, or one emitted instruction sequence.

## Acceptance Criteria

- The MOVI sign-extension mismatch is localized to a concrete generated-code
  owner with evidence from generated artifacts or focused dumps.
- The repair is a general AArch64 integer zero-extension or immediate
  materialization capability, not a named representative shortcut.
- Focused backend coverage fails without the repair and passes with it.
- The focused `00204.c` representative either passes or advances to a newly
  classified first bad fact.
- Adjacent repaired AArch64 variadic guardrails remain stable.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00204.c`, the MOVI block, `abcd0000`, `aaaaaaaa`, one
  register, one stack offset, one source line, or one emitted instruction
  sequence instead of repairing a general AArch64 integer materialization
  owner;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, or CTest registration to hide the failure;
- claims MOVI progress without generated-code evidence identifying where
  zero-extension becomes sign-extension;
- reopens HFA/floating, byval aggregate, stdarg cursor, fixed-formal,
  local/value-home, or frame/formal owners without direct evidence tying the
  current first bad fact to that owner;
- adds only external `00204.c` proof without focused backend coverage for the
  repaired MOVI or integer materialization behavior.
