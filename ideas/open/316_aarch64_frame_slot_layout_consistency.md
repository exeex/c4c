# AArch64 Frame Slot Layout Consistency

Status: Open
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

## Reactivation Evidence

Reactivated on 2026-05-20 from idea 350's unsigned div/rem proof. After the
unsigned div/rem producer publication repair, `00182` advanced from a runtime
mismatch to a runtime segfault. The fresh first bad fact is a caller local-array
frame-size/layout divergence: `main` declares `char buf[5*MAX_DIGITS]` with
`MAX_DIGITS == 32`, so the buffer needs 160 bytes, but generated AArch64
allocates only 48 bytes, passes `sp` as the buffer argument to `print_led`, and
saves `x30` at `[sp, #24]`. Once `print_led` writes the LED output into that
undersized caller frame, it overwrites the saved return address with output
bytes.

This evidence replaces the stale `00216.c`-only trigger as the current
activation signal. `00216.c` remains historical context for the same general
frame-size/frame-slot consistency owner, but the next activation should start
from the current `00182` local array frame allocation failure.

## Goal

Repair AArch64 frame-slot/frame-layout consistency when generated local
aggregate or stack-slot storage can exceed the frame allocation reserved by the
function prologue.

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
  diverge for the current `00182.c` local array representative and any still
  relevant focused `00216.c` large-slot evidence.
- Repair the frame-layout contract so emitted stack-slot accesses fit within
  the frame allocation or are otherwise proven to target valid owned storage.
- Ensure local arrays such as `char buf[5*MAX_DIGITS]` reserve enough caller
  frame space before their address is passed to callees that write through the
  buffer.
- Preserve frame-slot identity, width, alignment, call-boundary behavior, and
  existing legal stack-offset materialization from idea 314.
- Add focused backend or prepared-layout coverage that proves frame size and
  emitted local aggregate or slot offsets stay consistent.
- Use `c_testsuite_aarch64_backend_src_00182_c` as the current representative
  external case for the local-array frame-size residual.

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
- Unsigned div/rem producer publication, digit extraction semantics, LED output
  formatting, or any `00182` repair that bypasses the local array frame
  allocation/layout contract.

## Acceptance Criteria

- Focused backend or prepared-layout coverage proves emitted AArch64 local
  aggregate and stack-slot storage fit within the frame allocation.
- `c_testsuite_aarch64_backend_src_00182_c` advances past the current segfault
  caused by `main` under-allocating the local `buf` frame storage, or any later
  first-bad fact is classified as outside this owner.
- Relevant `00216.c` frame-size/slot-offset evidence is either covered by the
  same repaired layout contract or explicitly reclassified as stale/unrelated.
- Existing idea 312, 313, and 314 focused guardrails remain green.
- Fresh build and focused CTest proof are recorded before closure.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00216.c`, `test_correct_filling`, offsets `1600`, `1624`,
  `1644`, `1648`, `00182.c`, `print_led`, `MAX_DIGITS`, `buf`, one stack slot,
  one function, or one expected-output mismatch instead of repairing the
  frame-size/local-storage consistency contract;
- changes expectations, unsupported classifications, allowlists, timeout
  policy, runner behavior, CTest registration, proof-log contents, or test
  contracts to improve counts;
- claims progress while generated code can still access local aggregate storage
  or frame slots beyond the prologue allocation through the same layout path;
- folds large frame setup/teardown materialization, scalar stack publication,
  unsigned div/rem producer publication, f128 transport, semantic admission, or
  unrelated runtime mismatch work into this owner;
- hides the old out-of-frame access behind new helper names without proving the
  frame allocation and emitted local storage offsets agree.
