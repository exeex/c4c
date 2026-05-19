# AArch64 Variadic Va Start Helper Lowering

Status: Open
Created: 2026-05-19
Split From: ideas/closed/315_aarch64_large_frame_adjustment_materialization.md

## Goal

Lower AArch64 `va_start` helper payloads into legal selected assembly instead
of emitting raw `va.start` note text into `.s` output.

## Why This Exists

After idea 315 repaired large frame setup and teardown materialization, the
`00204.c` representative advanced to assembler failures in `myprintf`. The
generated assembly contains raw helper lines such as:

```text
va.start dest=value#2051:register:x13 ...
va.start.rsa slot#2569 stack+672 ...
va.start.initial_offsets gp=-56 fp=-128 ...
va.start.field kind=overflow_arg_area offset=0 size=8
```

These lines are backend helper payloads, not AArch64 assembly. Evidence points
to `src/backend/mir/aarch64/codegen/variadic.cpp::print_variadic_call` for
`PreparedVariadicEntryHelperKind::VaStart`.

## In Scope

- Inspect the prepared variadic helper representation for AArch64 `va_start`.
- Replace raw `va.start` payload printing with legal selected instructions for
  initializing the target `va_list` fields.
- Preserve the existing prepared handoff facts and note-level guardrails that
  describe variadic metadata before machine printing.
- Add focused backend coverage for AArch64 `va_start` helper lowering.
- Use `c_testsuite_aarch64_backend_src_00204_c` as the representative external
  case for the raw helper-text residual.

## Out Of Scope

- Reopening idea 315's large frame setup and teardown materialization owner.
- Reopening idea 314's stack-slot memory or scalar stack-publication spelling.
- Repairing idea 316's frame-slot/frame-layout consistency residual.
- Repairing scalar ALU immediate materialization such as `mov w9, #503808`;
  that belongs to `ideas/open/318_aarch64_scalar_alu_immediate_materialization.md`.
- Changing semantic admission, variadic prepared-handoff contracts, runners,
  timeout policy, expectations, unsupported classifications, or CTest
  registration.
- Filename-only, function-name-only, literal-stack-offset-only, or
  c-testsuite-number-specific fixes.

## Acceptance Criteria

- AArch64 `va_start` helper printing emits legal assembly, not raw `va.start`
  payload text.
- Focused backend coverage proves representative `va_list` field
  initialization behavior without weakening prepared helper metadata tests.
- `c_testsuite_aarch64_backend_src_00204_c` advances past the raw `va.start`
  assembler failures, or any later first-bad fact is classified as outside this
  owner.
- Existing idea 312, 314, and 315 focused guardrails remain green.
- Fresh build and focused CTest proof are recorded before closure.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00204.c`, `myprintf`, one helper id, one stack slot, one
  diagnostic, or one generated line instead of lowering general AArch64
  `va_start` helper payloads;
- deletes or hides `va.start` notes without emitting the required legal
  `va_list` initialization instructions;
- changes expectations, unsupported classifications, allowlists, timeout
  policy, runner behavior, CTest registration, proof-log contents, or test
  contracts to improve counts;
- claims progress through helper renames, note formatting changes, or
  classification-only changes while raw helper text can still reach assembly;
- folds large-frame materialization, scalar ALU immediate materialization,
  frame-layout consistency, or unrelated runtime mismatch work into this owner.
