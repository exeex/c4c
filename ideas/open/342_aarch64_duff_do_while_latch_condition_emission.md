# AArch64 Duff Do-While Latch Condition Emission

Status: Open
Created: 2026-05-20
Split From: ideas/closed/341_aarch64_fallthrough_fixed_offset_local_load_store_emission.md

## Goal

Repair the AArch64 generated-code residual where the Duff-device do-while latch
condition decrements `n` twice in the branch decision path, causing one fewer
loop iteration after the fallthrough copy emission has been repaired.

## Why This Exists

Idea 341 repaired the fixed-offset fallthrough local load/store emission gap for
`00143`: generated assembly no longer has the formerly empty copy labels
`.LBB1_8` through `.LBB1_20`, and those blocks now emit the expected `ldrh` and
`strh` data movement.

The representative still fails `[RUNTIME_NONZERO]`. The new first bad generated
assembly fact is in the Duff do-while latch: `.LBB1_29` stores `--n`, then
reloads `n`, subtracts one again, and branches on that second `n - 1` compare.
This terminates the Duff loop one iteration early. That residual is distinct
from fixed-offset fallthrough load/store emission.

## In Scope

- Localize where the do-while latch condition for the Duff loop is lowered or
  emitted with a duplicate decrement.
- Repair the semantic lowering, compare selection, branch emission, or
  writeback path so the latch branches on the post-decrement value exactly once.
- Add or update focused backend coverage for a do-while latch with decrement
  writeback and branch condition behavior.
- Re-run the representative
  `c_testsuite_aarch64_backend_src_00143_c`.

## Out Of Scope

- Fixed-offset fallthrough local load/store copy emission repaired by idea 341.
- Scalar-cast register-source publication repaired by idea 340.
- Expectation, unsupported, runner, timeout, proof-log-policy, or CTest
  registration changes.
- Fixing only `00143`, `.LBB1_29`, one source line, one local name, one block
  number, or one emitted instruction spelling.
- Broad frame-layout, aggregate, variadic, parser, semantic HIR, or frontend
  rewrites unless fresh evidence proves they own this latch-condition residual.

## Acceptance Criteria

- The first bad boundary is localized to a concrete do-while latch lowering,
  condition materialization, compare selection, branch emission, or writeback
  owner.
- Focused backend coverage proves the latch condition does not apply a second
  decrement after storing the post-decrement loop counter.
- `c_testsuite_aarch64_backend_src_00143_c` either passes or is reclassified by
  a new first bad fact after the duplicate latch decrement is repaired.
- No expectation, runner, timeout, unsupported classification,
  CTest-registration, or proof-log-policy change is used to claim progress.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00143`, `.LBB1_29`, Duff-device labels, selected source lines,
  local name `n`, one block number, or one exact instruction sequence instead
  of repairing latch-condition semantics generally;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, or CTest registration to reduce the failure count;
- claims progress only through helper renames, diagnostic rewrites,
  classification notes, or c-testsuite expectation changes;
- reopens fixed-offset fallthrough copy emission or scalar-cast source
  publication without fresh evidence that those old first bad facts returned;
- broadens into unrelated frame-layout, aggregate, variadic, parser, semantic
  HIR, or frontend rewrites before proving the latch-condition boundary;
- leaves generated AArch64 still storing the decremented counter and then
  branching on a second decremented value behind a renamed abstraction.
