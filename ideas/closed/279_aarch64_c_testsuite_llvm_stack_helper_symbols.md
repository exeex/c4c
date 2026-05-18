# AArch64 C-Testsuite LLVM Stack Helper Symbols

Status: Closed
Created: 2026-05-18
Closed: 2026-05-18

## Intent

Repair the AArch64 backend failure family where backend-route c-testsuite cases
emit unresolved LLVM stack helper symbols such as `llvm.stacksave`,
`llvm.dynamic_alloca.i8`, and `llvm.stackrestore`.

## Why This Exists

The undefined temporary-label route closed after the broad AArch64 backend scan
reported zero remaining `.LBB...` diagnostics. That same scan left one
non-runtime backend failure, `00207.c`, caused by undefined LLVM stack helper
references. This is a distinct backend-symbol or lowering capability from
temporary block-label emission and from runtime-runner availability.

## In Scope

- Reproduce and classify the representative `00207.c` AArch64 backend failure.
- Trace where LLVM stack helper intrinsics enter the backend route and which
  layer should lower, rewrite, or reject them.
- Implement a general backend rule for stack-save, dynamic-allocation, and
  stack-restore helper handling when that route is meant to support the case.
- Preserve truthful stage-specific diagnostics if a helper cannot yet be
  lowered.
- Prove the repair with focused backend tests plus the relevant AArch64
  c-testsuite backend subset.

## Out of Scope

- Configuring an AArch64 runtime runner or claiming runtime pass evidence.
- Reopening completed call-boundary move or undefined temporary-label work
  unless those exact old diagnostics return.
- Weakening expectations, allowlists, unsupported classifications, or
  stage-specific diagnostics.
- Adding filename-specific handling for `00207.c`.
- Broad rewrites of unrelated AArch64 ABI, call lowering, or frontend paths.

## Acceptance Criteria

- The representative LLVM stack helper failure no longer emits unresolved
  helper symbol references when the backend route claims to compile the case.
- Any lowering or rejection rule is general over the helper family, not tied to
  one c-testsuite filename.
- Focused backend proof demonstrates the helper-symbol handling path.
- The relevant AArch64 c-testsuite backend subset reclassifies the targeted
  failure to the next truthful owner layer without counting
  `[RUNTIME_UNAVAILABLE]` as pass evidence.

## Reviewer Reject Signals

Reject the route or slice if it:

- matches `00207.c` or exact helper spellings without repairing the helper
  lowering or symbol-authority rule;
- only rewrites expectations, allowlists, unsupported classifications, or
  diagnostics while unresolved LLVM helper references remain in generated
  output;
- claims runtime capability progress without an AArch64 runner or native host;
- collapses backend, assembler/link, and runtime failures into one generic
  failure bucket;
- reintroduces the completed undefined `.LBB...` temporary-label failure mode;
- broadly rewrites unrelated ABI, call, or frontend lowering paths to mask this
  helper-symbol family.

## Completion Notes

Closed after focused backend proof passed and the broad AArch64 backend scan
reported zero remaining unresolved `llvm.stacksave`,
`llvm.dynamic_alloca.*`, or `llvm.stackrestore` references. The representative
`00207.c` case now reaches the next truthful owner layer as an explicit
dynamic-stack unsupported diagnostic instead of unresolved helper calls.

`[RUNTIME_UNAVAILABLE]` results were not counted as pass evidence. Full
AArch64 dynamic-stack lowering remains a separate capability family tracked by
`ideas/open/280_aarch64_dynamic_stack_lowering.md`.
