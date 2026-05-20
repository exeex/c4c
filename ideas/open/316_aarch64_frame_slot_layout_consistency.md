# AArch64 Frame Slot Layout Consistency

Status: Parked
Created: 2026-05-19
Split From: ideas/closed/314_aarch64_large_stack_offset_addressing.md

## Parking Note

Parked on 2026-05-20 after Step 1 localization found the recorded source
evidence stale in current generated artifacts. Current `00216.c` artifacts for
`test_correct_filling`, `test_zero_init`, and `foo` show coherent prepared
frame sizes, slot offsets, emitted prologue allocations, and accessed
addresses; no frame-layout owner could be localized from the stale
`sub sp, sp, #48` / high-offset evidence. Reactivate only with fresh failing
proof that shows a current AArch64 frame-size/frame-slot layout divergence.

## Goal

Repair AArch64 frame-slot/frame-layout consistency when generated stack-slot
accesses target offsets outside the frame allocation reserved by the function
prologue.

## Why This Exists

Idea 314 repaired illegal large stack-offset instruction spelling. The
`00216.c` representative advanced past the assembler rejection of:

```asm
ldr x13, [sp, #1644]
```

The same representative now reaches execution and fails with a runtime output
mismatch. Generated-code evidence points to a separate layout consistency
problem: `test_correct_filling` reserves only 48 bytes with
`sub sp, sp, #48`, while the function still emits stack accesses through large
offsets such as `sp + 1600`, `sp + 1624`, `sp + 1644`, and `sp + 1648`.

This owner is about making the frame allocation and selected frame-slot offsets
agree. The large-offset instructions are now legal to spell; they are not yet
proven to address storage inside the function's frame.

## In Scope

- Localize where AArch64 frame size, frame slots, and selected stack offsets
  diverge for the `00216.c` representative.
- Repair the frame-layout contract so emitted stack-slot accesses fit within
  the frame allocation or are otherwise proven to target valid owned storage.
- Preserve frame-slot identity, width, alignment, call-boundary behavior, and
  existing legal stack-offset materialization from idea 314.
- Add focused backend or prepared-layout coverage that proves frame size and
  emitted slot offsets stay consistent.
- Use `c_testsuite_aarch64_backend_src_00216_c` as the representative external
  case for the frame-slot/frame-layout residual.

## Out Of Scope

- Reopening idea 314's instruction-spelling owner for large stack-slot memory
  operations or scalar stack publications.
- Repairing `00204.c` large frame setup/teardown materialization.
- Reopening semantic prepared-handoff, f128 transport, direct-call shuffle,
  vararg admission, runner, timeout, expectation, unsupported-classification,
  or CTest registration behavior.
- Broad frame-layout rewrites that are not driven by focused evidence of the
  frame-size/slot-offset divergence.
- Filename-only, function-name-only, literal-offset-only,
  diagnostic-string-only, or c-testsuite-number-specific fixes.

## Acceptance Criteria

- Focused backend or prepared-layout coverage proves emitted AArch64 stack-slot
  offsets fit the frame allocation for large-offset cases.
- `c_testsuite_aarch64_backend_src_00216_c` advances past the current runtime
  mismatch caused by frame-size/slot-offset divergence, or any later first-bad
  fact is classified as outside this owner.
- Existing idea 312, 313, and 314 focused guardrails remain green.
- Fresh build and focused CTest proof are recorded before closure.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00216.c`, `test_correct_filling`, offsets `1600`, `1624`,
  `1644`, `1648`, one stack slot, one function, or one expected-output mismatch
  instead of repairing the frame-size/slot-offset consistency contract;
- changes expectations, unsupported classifications, allowlists, timeout
  policy, runner behavior, CTest registration, proof-log contents, or test
  contracts to improve counts;
- claims progress while generated code can still access frame slots beyond the
  prologue allocation through the same layout path;
- folds large frame setup/teardown materialization, scalar stack publication,
  f128 transport, semantic admission, or unrelated runtime mismatch work into
  this owner;
- hides the old out-of-frame access behind new helper names without proving the
  frame allocation and emitted offsets agree.
