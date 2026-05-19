# AArch64 Scalar Alu Immediate Materialization

Status: Closed
Created: 2026-05-19
Split From: ideas/closed/315_aarch64_large_frame_adjustment_materialization.md
Closed: 2026-05-19

## Goal

Materialize non-encodable scalar ALU immediates through legal AArch64 constant
sequences before arithmetic instructions.

## Why This Exists

After idea 315 repaired large frame setup and teardown materialization, the
`00204.c` representative also exposes an assembler failure in the scalar helper
`subim503808`:

```text
mov w9, #503808
```

That immediate is not legal for the printed `mov` form. The local sequence is
scalar arithmetic helper code, not frame setup or teardown. Evidence points to
`src/backend/mir/aarch64/codegen/alu.cpp`, where non-plain scalar immediates
can still be emitted through a single `mov <scratch>, #<imm>` instead of a
legal materialization helper sequence.

## In Scope

- Inspect AArch64 scalar ALU immediate paths that publish non-encodable
  constants through scratch registers.
- Route non-encodable ALU constants through legal integer materialization
  sequences before the arithmetic operation.
- Preserve direct encodable immediate and register-register ALU paths.
- Add focused backend coverage for at least one non-encodable 32-bit scalar ALU
  constant and the existing encodable paths.
- Use `c_testsuite_aarch64_backend_src_00204_c` as a representative external
  case after earlier assembler blockers have advanced.

## Out Of Scope

- Reopening idea 315's large frame setup and teardown materialization owner.
- Reopening idea 314's stack-slot memory or scalar stack-publication spelling.
- Lowering raw `va.start` helper text; that belongs to
  `ideas/open/317_aarch64_variadic_va_start_helper_lowering.md`.
- Repairing idea 316's frame-slot/frame-layout consistency residual.
- Changing semantic admission, runners, timeout policy, expectations,
  unsupported classifications, CTest registration, or proof-log policy.
- Filename-only, function-name-only, literal `503808`-only, diagnostic-only, or
  c-testsuite-number-specific fixes.

## Acceptance Criteria

- Non-encodable scalar ALU immediates are materialized through legal AArch64
  instruction sequences before use.
- Existing encodable scalar ALU immediates remain on the direct path.
- Focused backend coverage proves both the non-encodable materialization path
  and preservation of existing direct behavior.
- `c_testsuite_aarch64_backend_src_00204_c` advances past the illegal
  `mov w9, #503808` assembler failure, once earlier unrelated assembler
  blockers are cleared, or any later first-bad fact is classified outside this
  owner.
- Fresh build and focused CTest proof are recorded before closure.

## Closure Notes

Idea 318 is complete. The focused proof advances past the former assembler
diagnostic: generated `subim503808` no longer emits illegal
`mov w9, #503808`, and instead materializes the constant through legal
`movz`/`movk` instructions before the scalar `sub`.

The remaining focused `00204.c` failure is runtime-only:
`RUNTIME_NONZERO` / `Segmentation fault` during early `Arguments:` output.
The output shows corrupted floating/HFA/aggregate argument values before
execution reaches `Return values:`, `stdarg:`, `MOVI:`, or the `opi()` scalar
helper section that contains `subim503808`. That first bad fact is outside
this scalar ALU immediate materialization owner and is tracked by
`ideas/open/319_aarch64_hfa_aggregate_argument_runtime.md`.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00204.c`, `subim503808`, literal `503808`, one scratch
  register, one diagnostic, or one emitted line instead of repairing general
  scalar ALU immediate materialization;
- changes expectations, unsupported classifications, allowlists, timeout
  policy, runner behavior, CTest registration, proof-log contents, or test
  contracts to improve counts;
- claims progress through formatting or helper renames while illegal scalar
  ALU immediates can still reach assembly;
- routes encodable immediates through unnecessary broad rewrites without a
  contract reason;
- folds raw variadic helper lowering, large-frame materialization, frame-layout
  consistency, or unrelated runtime mismatch work into this owner.
