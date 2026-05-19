# AArch64 Aggregate Va Arg Helper Lowering

Status: Open
Created: 2026-05-19
Split From: ideas/closed/320_aarch64_f128_transport_addressability.md

## Goal

Lower AArch64 aggregate `va_arg` helper machine records into executable
copy/progression code so generated assembly no longer contains raw
`va.arg.aggregate` descriptive text.

## Why This Exists

Idea 320 repaired the `f128_transport` addressability blocker for the `00204.c`
representative. The focused proof now gets past that machine-printer
diagnostic, but the assembler rejects raw aggregate variadic helper records
such as `va.arg.aggregate`, `va.arg.aggregate.source`, and
`va.arg.aggregate.progress`.

The current owner is the AArch64 variadic aggregate helper printing/lowering
surface, not F128 transport addressability, HFA argument classification, scalar
ALU immediates, frame materialization, stack-slot spelling, global initializer
emission, runners, or test expectations.

## In Scope

- Localize the `PreparedVariadicEntryHelperKind::VaArgAggregate` machine path
  that currently prints descriptive helper text.
- Lower `VariadicVaArgAggregate` helper records into executable AArch64
  source selection, aggregate copy, and `va_list` progression code.
- Preserve the HFA lane repair, variadic HFA lane handoff, callee aggregate
  handoff, F128 transport addressability, scalar ALU immediate materialization,
  frame adjustment materialization, and stack-offset spelling from prior ideas.
- Add focused backend coverage for aggregate `va_arg` helper output that
  asserts executable assembly, not raw helper records.
- Use `c_testsuite_aarch64_backend_src_00204_c` as the representative external
  proof once local helper output is repaired.

## Out Of Scope

- Reopening F128 transport addressability from idea 320.
- Reopening HFA, floating, long-double, or aggregate argument classification
  and call-lane lowering from idea 319.
- Reopening scalar ALU immediate materialization, raw `va_start` lowering,
  large frame adjustment materialization, large stack-offset spelling, or
  frame-slot/frame-layout consistency.
- Fixing global initializer emission unless it becomes the next first bad fact
  after aggregate `va_arg` helper text is gone.
- Changing semantic admission, unsupported classifications, expectations,
  CTest registration, runner behavior, timeout policy, or proof-log policy.

## Acceptance Criteria

- The aggregate `va_arg` helper source, copy, and progression records are
  localized to concrete AArch64 backend code surfaces.
- Raw `va.arg.aggregate` helper text no longer reaches emitted assembly.
- The repair is a general aggregate `va_arg` helper lowering rule, not a
  `00204.c`, `stdarg`, `myprintf`, one-struct, one-register, or one-slot
  shortcut.
- Focused backend coverage exercises the aggregate helper output and rejects
  raw helper mnemonics.
- Prior-owner guardrails remain cleared under a fresh build and focused CTest
  proof.
- If `00204.c` advances to another first bad fact outside aggregate `va_arg`
  helper lowering, the residual is classified into a separate owner.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00204.c`, `stdarg`, `myprintf`, one HFA struct, one
  register, one stack slot, one helper index, or one emitted line instead of
  lowering aggregate `va_arg` helpers generally;
- hides the failure by commenting out helper records, renaming helper
  mnemonics, weakening assertions, or changing expectations/unsupported
  classifications without executable AArch64 output;
- claims capability progress while raw `va.arg.aggregate`,
  `va.arg.aggregate.source`, or `va.arg.aggregate.progress` still reaches
  assembly for the focused representative;
- folds global initializer emission, F128 transport, HFA argument ABI,
  frame-layout consistency, scalar ALU immediates, stack-offset spelling, or
  runner policy into this owner without generated-code evidence tying that work
  to aggregate `va_arg` helper lowering;
- adds coverage that only checks the c-testsuite representative while omitting
  focused backend coverage for the aggregate helper lowering contract.
