# AArch64 C-Testsuite 00205 Timeout Residual

Status: Open
Created: 2026-05-19
Split From: ideas/open/303_aarch64_sign_extension_assembler_legality.md

## Goal

Classify and repair the backend-generated-code cause of the focused
`c_testsuite_aarch64_backend_src_00205_c` timeout exposed after the
sign-extension assembler-legality repair.

## Why This Exists

Idea 303 repaired the illegal AArch64 sign-extension spelling in the focused
`00205` route: generated assembly now contains legal `sxtw x9, w13` instead of
illegal `sxtw w9, w13`.

The same focused testcase now proceeds past assembler validation and times out
after 5.01 seconds. That is a separate runtime or generated-control-flow
residual, not a sign-extension spelling failure. It needs its own owner so the
legality repair is preserved without silently expanding idea 303 into timeout,
runner, or runtime-policy work.

## In Scope

- Generated AArch64 code, control-flow, loop, compare, branch, and value-flow
  inspection for the focused `00205` timeout.
- Narrow runtime probes that explain why the generated binary does not finish
  within the existing test timeout.
- A semantic backend repair if the timeout traces to a generated-code bug in
  branch lowering, compare lowering, value materialization, or related
  AArch64 execution semantics.
- Splitting a more precise follow-up idea if inspection proves the timeout
  belongs to a larger semantic family outside this singleton.
- Fresh build proof plus the supervisor-selected focused backend subset.

## Out Of Scope

- Reopening idea 303's sign-extension spelling repair unless generated
  assembly regresses to illegal `sxtw` destination width.
- Claiming pass-count progress while `00205` still times out.
- Timeout threshold changes, runner behavior changes, stale-process policy,
  expectation rewrites, allowlist changes, unsupported classifications, or
  CTest registration changes.
- Filename-only fixes or shortcuts that make `00205` finish without repairing
  the underlying generated-code semantics.
- Broad backend rewrites before the timeout mechanism is classified.

## Acceptance Criteria

- The timeout mechanism is classified from generated assembly, runtime probes,
  or a focused backend trace rather than from testcase count movement alone.
- If repaired under this owner, the repair addresses a semantic generated-code
  cause and the focused proof no longer times out for the same reason.
- If inspection shows a wider or different owner is required, this idea records
  the evidence and lifecycle switches to that focused owner instead of
  absorbing unrelated work.
- Focused proof preserves the accepted sign-extension legality result:
  generated assembly must not return to illegal `sxtw` with a W destination.
- No expectation, allowlist, unsupported classification, timeout policy,
  runner behavior, proof-log policy, or CTest registration changes are used as
  capability progress.

## Reviewer Reject Signals

Reject the route if it:

- special-cases the filename `00205`, source line numbers, temporary labels, or
  exact register names instead of repairing a semantic generated-code cause;
- changes timeout thresholds, runner behavior, stale-process handling,
  expectations, allowlists, unsupported classifications, or CTest registration
  while claiming backend capability progress;
- claims pass-count progress when the focused testcase still times out or when
  the only evidence is a classification/expectation change;
- regresses the idea 303 legality result by reintroducing illegal `sxtw` with a
  32-bit destination register;
- performs a broad compare-branch, loop, ABI, runtime, or libc rewrite before
  the timeout mechanism is classified from generated code or runtime evidence;
- hides the same non-terminating generated behavior behind a helper rename,
  test harness adjustment, or narrower proof selection.
