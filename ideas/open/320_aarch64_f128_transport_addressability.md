# AArch64 F128 Transport Addressability

Status: Open
Created: 2026-05-19
Split From: ideas/closed/319_aarch64_hfa_aggregate_argument_runtime.md

## Goal

Repair AArch64 `f128_transport` machine-node addressability and printing so
the `00204.c` representative can advance past the current machine-printer
blocker after HFA and aggregate argument lowering has handed off correctly.

## Why This Exists

Idea 319 repaired the HFA and aggregate argument ABI path enough for the
representative to move beyond early `Arguments:` corruption and the prior
runtime segmentation fault. The focused proof now reaches the AArch64
machine-node printer and fails because an `f128_transport` memory transport
address cannot be printed.

The local owner is the backend machine-node transport/addressability surface,
not argument ABI classification, variadic helper lowering, scalar ALU
immediate materialization, frame adjustment materialization, or large stack
offset spelling.

## In Scope

- Localize the `f128_transport` record, carrier, call-boundary move, or printer
  path that creates an unprintable memory address.
- Repair general AArch64 `fp128`/`f128_transport` address materialization or
  printing for memory transports.
- Preserve the fixed HFA lane lowering, variadic HFA lane handoff, and
  aggregate `va_arg` helper handoff from idea 319.
- Add focused backend coverage for the repaired `f128_transport`
  addressability path.
- Use `c_testsuite_aarch64_backend_src_00204_c` as the representative external
  proof after prior blockers remain cleared.

## Out Of Scope

- Reopening idea 319's HFA, floating, long-double, or aggregate ABI argument
  classification and lowering owner.
- Reopening idea 318's scalar ALU immediate materialization owner.
- Reopening idea 317's raw `va_start` helper-text lowering owner.
- Reopening idea 315's large frame setup and teardown materialization owner.
- Reopening idea 314's stack-slot memory or scalar stack-publication spelling.
- Repairing idea 316's frame-slot/frame-layout consistency residual unless
  generated evidence proves it is the same `f128_transport` addressability
  fault.
- Changing semantic admission, runners, timeout policy, expectations,
  unsupported classifications, CTest registration, or proof-log policy.
- Fixing global initializer emission unless it becomes the next first bad fact
  after `f128_transport` addressability is repaired.

## Acceptance Criteria

- The failing `f128_transport` machine node is localized to a concrete AArch64
  transport, carrier, call-boundary, or printer owner.
- The `fp128` memory transport address is printed or materialized through a
  general backend rule, not a `00204.c`-specific shortcut.
- Focused backend coverage exercises the repaired `f128_transport`
  addressability path.
- Prior HFA/aggregate argument ABI and assembler-blocker guardrails remain
  cleared.
- Fresh build and focused CTest proof are recorded before closure.
- If `00204.c` advances to a later first bad fact outside `f128_transport`
  addressability, that residual is classified into a separate owner instead of
  being folded into this idea.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00204.c`, `stdarg`, `myprintf`, one HFA struct, one register,
  one stack slot, or one instruction index instead of repairing general
  `f128_transport` addressability;
- changes expectations, unsupported classifications, allowlists, timeout
  policy, runner behavior, CTest registration, proof-log contents, or test
  contracts to improve counts;
- claims progress while the same `f128 memory transport address is not
  printable` failure remains behind a renamed helper or reformatted diagnostic;
- folds HFA argument ABI lowering, scalar ALU immediates, raw variadic helper
  text, frame adjustment, large stack-offset spelling, or global initializer
  emission into this owner without generated-code evidence tying it to
  `f128_transport` addressability;
- adds coverage that only checks the external c-testsuite representative while
  omitting focused backend coverage for the repaired transport path.
