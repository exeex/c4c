# AArch64 Callee-Saved Scalar Home Preservation

Status: Closed
Created: 2026-05-20
Split From: ideas/closed/336_aarch64_return_result_publication_epilogue_clobber.md

## Goal

Repair the AArch64 backend path where a scalar value is kept in a callee-saved
physical register across a call, but the generated frame does not preserve that
register, so the caller's live scalar home is overwritten by the callee.

## Why This Exists

Closing idea 336 removed the stale return-result overwrite in `00168`.
Recursive `factorial` now computes the recursive result into `w0` and returns
directly from `w0`. The remaining mismatch is different: the caller keeps
`n` in `w19` across `bl factorial`, but the function prologue/epilogue does
not save and restore `x19`. After recursion returns, `w19` contains the
callee's value `1`, so every multiplication produces `1`.

This is not a return-publication problem. The return value in `w0` is no
longer overwritten by a stale terminal return move. The new owner is the
connection between scalar-home allocation, call liveness, and AArch64 frame
callee-saved preservation.

## In Scope

- Localize where AArch64 scalar homes are assigned to callee-saved registers
  such as `x19` through `x28`.
- Determine whether the missing preservation belongs to live-range analysis,
  physical register assignment, frame planning, prologue/epilogue emission, or
  the handoff between those layers.
- Repair the general rule that records or preserves callee-saved scalar homes
  that remain live across calls.
- Add focused backend coverage for a live scalar home in a callee-saved
  register across a direct or recursive call.
- Validate `00168` or an equivalent recursive-call representative after the
  frame-preservation repair.

## Out Of Scope

- Reopening return-result publication, terminal return self-move suppression,
  or before-return FunctionReturnAbi publication from idea 336.
- Fixing `00168` by matching the filename, function name `factorial`, register
  string `w19`/`x19`, multiplication shape, recursion shape, or expected output.
- Changing c-testsuite expectations, unsupported classifications, runner
  behavior, timeout policy, CTest registration, or proof-log handling.
- Broad-rewriting AArch64 register allocation, frame layout, or call lowering
  beyond what localization proves is needed for live callee-saved homes.
- Repairing unrelated scalar conversion/comparison, switch/control-flow,
  varargs/libc ABI, indirect-call pointer materialization, or scalar-cast
  machine-printer buckets from the umbrella inventory.

## Acceptance Criteria

- The first bad fact is localized to a concrete missing-preservation owner for
  a scalar home that is live across a call in a callee-saved register.
- A general repair ensures any such callee-saved physical register is either
  saved/restored by the frame or avoided/spilled according to the backend's
  intended ownership model.
- Focused backend coverage fails without the repair and passes with it.
- `c_testsuite_aarch64_backend_src_00168_c` passes or advances beyond the
  missing `x19` preservation fact with any new residual clearly classified.
- Adjacent AArch64 frame, call-boundary, scalar-home, and return-publication
  guardrails remain stable.

## Closure Note

Closed: 2026-05-20

The active runbook localized the failure to the AArch64 prepared-to-codegen
scalar callee-saved placement handoff, then repaired scalar GPR and
AggregateAddress callee-saved prepared placements so frame planning and operand
emission use the same prepared AArch64 scalar callee-saved profile. The focused
coverage now pins saved-register frame-record conversion from the explicit
frame register name, direct prepared placement conversion, and structured
physical assignment preference.

Close-gate proof used matching focused before/after logs for:

```bash
ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(prepared_register_conversion|return_lowering)|backend_prepare_(frame_stack_call_contract|liveness)|c_testsuite_aarch64_backend_src_00168_c'
```

`test_before.log` at the parent repair baseline passed 4/5 and failed only
`c_testsuite_aarch64_backend_src_00168_c` with the old factorial runtime
mismatch. `test_after.log` at the repaired head passed 5/5. The monotonic
regression guard reported no new failures.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00168`, `factorial`, recursion, `w19`/`x19`, or the exact
  emitted multiplication sequence instead of repairing callee-saved
  preservation for live scalar homes generally;
- claims progress by changing expected output, unsupported classifications,
  runner behavior, timeout policy, CTest registration, proof-log policy, or
  allowlists;
- reintroduces the stale return-publication overwrite fixed by idea 336 or
  expands return-move suppression to hide the preservation failure;
- preserves every callee-saved register unconditionally without localization
  showing that this is the intended backend contract;
- only renames frame/register helpers, rewrites dumps, or moves the missing
  preservation behind another layer while live callee-saved scalar homes can
  still be clobbered across calls.
