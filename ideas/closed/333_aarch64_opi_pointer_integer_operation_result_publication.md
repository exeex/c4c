# AArch64 OPI Pointer Integer Operation Result Publication

Status: Closed
Created: 2026-05-20
Closed: 2026-05-20
Split From: ideas/open/332_aarch64_movi_zero_extension_materialization.md

## Goal

Classify and repair the AArch64 `opi()` residual where a simple pointer or
integer operation result is not the value observed by the following output
call.

## Why This Exists

Idea 332 repaired the MOVI zero-extension residual in commit `9ad547218`. The
focused MOVI guard passes and the representative now advances into `opi()`.
The first observed residual is at
`tests/c/external/c-testsuite/src/00204.c:476`: `pll(addip0(x))` expects
`3e8`, but the representative printed `c220ecc6`.

Generated AArch64 evidence points to operation result publication rather than
remaining MOVI materialization. `addip0(uint32_t x)` emits `add w0, w0, #0`
but then returns `mov x0, x13`; the caller also saves the returned `x0` to
`x20` and later publishes `x19` to `pll`. The next route should localize which
prepared/MIR result home, return register, call-result capture, or call-output
operand publication first loses the computed `1000` value.

## In Scope

- Localize the `opi()` first bad fact from source expression through BIR,
  prepared records, generated AArch64, return value capture, and the observing
  `pll` call.
- Determine whether the owner is scalar ALU result home publication,
  zero-width or no-op arithmetic lowering, return register selection,
  call-result capture, caller-side call-output publication, or another
  generated-code path with direct evidence.
- Repair the classified AArch64 operation result publication owner generally.
- Add focused backend coverage that fails without the repair and proves the
  repaired result/return/call-output path without relying only on the external
  `00204.c` representative.
- Preserve completed MOVI zero-extension, HFA/floating, byval aggregate,
  stdarg cursor, fixed-formal, local/value-home, and frame/formal repairs.

## Out Of Scope

- Reopening MOVI zero-extension or immediate cast folding unless fresh
  evidence shows a remaining MOVI mismatch.
- Reopening HFA/floating, byval aggregate lane publication, stdarg cursor,
  aggregate `va_arg`, fixed-formal entry publication, local/value-home
  publication, frame/formal publication, or large frame layout without direct
  generated-code evidence.
- Changing semantic admission, runners, timeout policy, expectations,
  unsupported classifications, proof-log policy, or CTest registration.
- Fixing only `00204.c`, `opi`, `addip0`, one literal value, one register,
  one source line, one stack slot, or one emitted instruction sequence.

## Acceptance Criteria

- The `addip0(x)` mismatch is localized to a concrete generated-code owner
  with evidence from generated artifacts or focused dumps.
- The repair is a general AArch64 scalar operation result, return, or
  call-output publication capability, not a named representative shortcut.
- Focused backend coverage fails without the repair and passes with it.
- The focused `00204.c` representative either passes or advances to a newly
  classified first bad fact for lifecycle handoff.
- Adjacent repaired AArch64 variadic, byval, MOVI, and publication guardrails
  remain stable.

## Closure Notes

Closed after the OPI representative advanced through and repaired the scalar
ALU result-home publication, caller call-result/cast publication,
rematerializable-immediate live-source clobbering, and direct scalar shift
immediate publication owners for `Shl`, `LShr`, and `AShr`.

Close proof:

- `test_before.log` to `test_after.log` regression guard passed: 10/11 to
  11/11, resolving `c_testsuite_aarch64_backend_src_00204_c` with no new
  failures.
- Supervisor broader backend validation passed:
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` was 141/141.

No durable follow-up under this source idea remains. Future AArch64 scalar
publication failures should open a new source idea unless direct evidence ties
them to one of the repaired owners above.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00204.c`, `opi`, `addip0`, `1000`, `3e8`, `c220ecc6`, one
  register, one stack offset, one source line, or one emitted instruction
  sequence instead of repairing a general AArch64 scalar result publication
  owner;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, or CTest registration to hide the failure;
- claims progress without generated-code evidence identifying where the
  computed operation result stops being the value published to the caller or
  to `pll`;
- reopens MOVI, HFA/floating, byval aggregate, stdarg cursor, fixed-formal,
  local/value-home, frame/formal, or large-frame owners without direct
  evidence tying the `opi()` first bad fact to that owner;
- adds only external `00204.c` proof without focused backend coverage for the
  repaired scalar result, return, or call-output publication behavior.
