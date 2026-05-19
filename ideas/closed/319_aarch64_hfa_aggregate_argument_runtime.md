# AArch64 HFA Aggregate Argument Runtime

Status: Closed
Created: 2026-05-19
Split From: ideas/closed/318_aarch64_scalar_alu_immediate_materialization.md

## Goal

Repair AArch64 runtime argument passing for HFA, floating, long-double, and
aggregate values enough for the `00204.c` representative to advance past the
early `Arguments:` corruption and segmentation fault.

## Why This Exists

After ideas 314, 315, 317, and 318 repaired earlier assembler blockers in the
same representative, `c_testsuite_aarch64_backend_src_00204_c` now builds and
runs far enough to fail at runtime. The latest focused proof shows
`RUNTIME_NONZERO` / `Segmentation fault` while printing the initial
`Arguments:` section. The output contains corrupted floating/HFA/aggregate
values and the program crashes before reaching `Return values:`, `stdarg:`,
`MOVI:`, or the `opi()` scalar arithmetic section.

That makes the next owner an ABI/runtime argument classification and lowering
problem, not scalar ALU immediate materialization.

## In Scope

- Localize the earliest corrupted `Arguments:` value in the generated AArch64
  call and callee paths.
- Inspect ABI classification and lowering for floating, long-double, HFA, and
  aggregate function arguments.
- Repair general AArch64 argument passing for the implicated ABI class, using
  structured records and existing lowering surfaces where possible.
- Add focused backend or ABI coverage that proves the repaired argument class
  without depending only on `00204.c`.
- Use `c_testsuite_aarch64_backend_src_00204_c` as the representative external
  runtime case after earlier assembler blockers remain cleared.

## Out Of Scope

- Reopening idea 318's scalar ALU immediate materialization owner.
- Reopening idea 317's raw `va_start` helper-text lowering owner.
- Reopening idea 315's large frame setup and teardown materialization owner.
- Reopening idea 314's stack-slot memory or scalar stack-publication spelling.
- Repairing idea 316's frame-slot/frame-layout consistency residual unless the
  localized ABI evidence proves the runtime corruption is the same fault.
- Changing semantic admission, runners, timeout policy, expectations,
  unsupported classifications, CTest registration, or proof-log policy.
- Filename-only, function-name-only, output-line-only, or c-testsuite-number
  specific fixes.

## Acceptance Criteria

- The earliest corrupted `Arguments:` value is localized to a concrete AArch64
  ABI classification or argument-lowering owner.
- The implicated HFA, floating, long-double, or aggregate argument path is
  repaired generally, not by special-casing `00204.c`.
- Focused backend or ABI coverage proves the repaired argument class.
- Prior assembler blockers remain cleared, including raw `va.start` text,
  large frame adjustment immediates, large stack-offset instruction spelling,
  and illegal scalar ALU `mov w9, #503808`.
- Fresh build and focused CTest proof are recorded before closure.
- If `00204.c` advances to a later first bad fact outside argument handling,
  that residual is classified into a separate owner instead of being folded
  into this idea.

## Closure Note

Closed: 2026-05-19

The fixed HFA aggregate argument path, caller-side variadic HFA lane handoff,
and callee-side aggregate `va_arg` helper handoff advanced the representative
past the original early `Arguments:` corruption and segmentation fault. The
remaining focused failure is now an AArch64 `f128_transport` machine-printer
addressability blocker:

`cannot print AArch64 machine node family=f128_transport opcode=f128_transport:
f128 memory transport address is not printable`

That residual is outside this idea's ABI argument classification and lowering
owner and is represented by
`ideas/open/320_aarch64_f128_transport_addressability.md`.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00204.c`, one printed `Arguments:` line, one function name, or
  one register/stack slot instead of repairing a general ABI argument class;
- changes expectations, unsupported classifications, allowlists, timeout
  policy, runner behavior, CTest registration, proof-log contents, or test
  contracts to improve counts;
- claims progress while the same early HFA/floating/aggregate argument
  corruption still appears behind a renamed helper or reformatted path;
- folds scalar ALU immediate materialization, raw variadic helper text, large
  frame adjustment, large stack-offset spelling, or unrelated later runtime
  work into this owner without generated-code evidence tying it to the
  argument ABI fault;
- accepts only the narrow `00204.c` result while nearby focused ABI coverage
  for the repaired argument class is missing or contradictory.
