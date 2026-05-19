# AArch64 Large Frame Adjustment Materialization

Status: Open
Created: 2026-05-19
Split From: ideas/closed/314_aarch64_large_stack_offset_addressing.md

## Goal

Repair AArch64 frame setup and teardown printing when a function frame size is
outside the plain `sub/add sp, sp, #imm` encoding range.

## Why This Exists

Idea 314 repaired large stack-slot memory addressing and stack-backed scalar
publication. The `00204.c` representative advanced past the scalar ALU
stack-publication printer failure and now stops at frame prologue printing:

```text
cannot print AArch64 machine node family=frame opcode=frame_setup:
frame adjustment immediate is outside the plain #imm encoding range 0..4095
```

The prepared dump for the `stdarg` representative reports `frame_size=5776`.
The current AArch64 frame printer rejects this before emitting the function
body because it only accepts the plain immediate range. This owner is about
legal materialization of large stack-pointer adjustments for frame setup and
teardown, not about selected stack-slot memory operands.

## In Scope

- Identify the AArch64 frame setup and teardown printing/lowering paths that
  emit `sub sp, sp, #frame_size` and `add sp, sp, #frame_size`.
- Materialize frame-size adjustments larger than the plain immediate range
  through legal AArch64 sequences.
- Preserve stack-pointer balance, call-frame behavior, and existing in-range
  frame adjustment output.
- Add focused backend coverage for large frame setup and teardown adjustments.
- Use `c_testsuite_aarch64_backend_src_00204_c` as the representative external
  case for the large frame-adjustment residual.

## Out Of Scope

- Reopening idea 314's stack-slot load/store or scalar stack-publication
  instruction-spelling owner.
- Repairing `00216.c` frame-slot/frame-layout consistency.
- Reopening semantic prepared-handoff, f128 transport, vararg admission, runner,
  timeout, expectation, unsupported-classification, or CTest registration
  behavior.
- Reworking frame layout or slot assignment unless focused evidence proves the
  frame-size value itself is wrong.
- Filename-only, function-name-only, literal `5776`-only, diagnostic-string-only,
  or c-testsuite-number-specific fixes.

## Acceptance Criteria

- Focused backend coverage proves large AArch64 frame setup and teardown
  adjustments use legal instruction sequences.
- Existing in-range frame setup and teardown output remains on the direct path.
- `c_testsuite_aarch64_backend_src_00204_c` advances past the current
  `frame_setup` immediate materialization failure, or any later first-bad fact
  is classified as outside this owner.
- Existing idea 312, 313, and 314 focused guardrails remain green.
- Fresh build and focused CTest proof are recorded before closure.

## Completion Notes

Closed on 2026-05-19. The focused proof for idea 315 no longer reports the
original `frame_setup` immediate materialization diagnostic, and backend
coverage now proves large frame setup and teardown materialization through legal
selected instructions while preserving direct small-frame output.

The remaining `00204.c` assembler residuals are outside this owner and were
split into follow-up ideas:

- `ideas/open/317_aarch64_variadic_va_start_helper_lowering.md` for raw
  `va.start` helper payload text that reaches generated assembly.
- `ideas/open/318_aarch64_scalar_alu_immediate_materialization.md` for illegal
  scalar ALU immediate assembly such as `mov w9, #503808`.

Close-time regression guard used the focused 315 proof command recorded in
`todo.md`. Non-decreasing close mode passed at 10 passed / 1 failed before and
10 passed / 1 failed after, with no new failing tests.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00204.c`, `stdarg`, `frame_size=5776`, one function name, one
  block, or one diagnostic instead of repairing general large frame adjustment
  materialization;
- changes expectations, unsupported classifications, allowlists, timeout
  policy, runner behavior, CTest registration, proof-log contents, or test
  contracts to improve counts;
- claims capability progress while large frame teardown remains illegal or the
  stack pointer is not balanced across setup and teardown;
- folds stack-slot/frame-layout consistency, scalar stack publication, f128
  transport, semantic admission, or unrelated runtime mismatch work into this
  owner;
- hides the same frame adjustment failure behind a helper rename or a weaker
  printer contract.
