# AArch64 Loop Branch Control Runtime Hang

Status: Open
Created: 2026-05-18
Discovered From: ideas/open/278_aarch64_backend_local_operand_materialization_runtime_nonzero.md

## Intent

Repair the AArch64 backend loop or conditional branch control lowering failure
exposed by `tests/c/external/c-testsuite/src/00006.c`, where generated
assembly unconditionally branches back to the loop label and hangs at runtime.

## Why This Exists

After idea 278 repaired scalar local operand materialization, the broad AArch64
backend scan reached `00006.c` and hung. The observed assembly decrements a
stack local but emits an unconditional branch back to `.LBB1_2`, leaving the
post-loop return unreachable.

This is a distinct later owner layer from the scalar local materialization
repair and from the address-exposed local failures in `00004.c` and `00005.c`.
It should be investigated as branch/loop control lowering, CFG edge selection,
or comparison-to-branch emission, not folded into idea 278.

## In Scope

- Trace the HIR/BIR/MIR or prepared control-flow facts for `00006.c`.
- Identify where the loop condition, comparison result, or branch target choice
  is lost or lowered as an unconditional self-loop.
- Repair the semantic control-flow lowering rule for the supported loop or
  conditional branch form proven by the trace.
- Preserve existing AArch64 return-result and local operand materialization
  behavior from ideas 277 and 278.
- Use timeout-bounded runtime proof or assembly-only localization while the hang
  remains active.

## Out of Scope

- Address-exposed local or pointer semantics for `00004.c` and `00005.c`.
- Changing c-testsuite `.expected` sidecars, allowlists, unsupported
  classifications, CTest contracts, runner files, or timeout policy to claim
  progress.
- Adding filename-specific handling for `00006.c`.
- Routing proof through LLVM IR fallback while claiming AArch64 backend
  progress.
- Broad CFG, parser, frontend, ABI, or register-allocation rewrites unless the
  trace proves the branch-control owner requires them.

## Completion Criteria

- `00006.c` no longer hangs due to an unconditional self-loop in generated
  AArch64 output.
- The generated control flow contains a truthful loop-exit path and reaches the
  intended return when the source condition is false.
- The repaired branch/loop rule is covered by focused backend proof, not only
  by the c-testsuite filename.
- Ideas 277 and 278 remain green for result-register placement and scalar local
  operand materialization.
- Any remaining later runtime failure is classified truthfully and split rather
  than hidden behind timeout, expectation, or runner changes.

## Reviewer Reject Signals

Reject the route or slice if it:

- matches `00006.c`, a c-testsuite path, or one exact label spelling instead of
  repairing loop or conditional branch control lowering;
- changes `.expected` files, allowlists, unsupported classifications, CTest
  contracts, runner files, or timeout policy to make the hang disappear;
- routes proof through LLVM IR fallback while claiming AArch64 backend progress;
- fixes only the visible self-loop while leaving equivalent conditional branch
  or loop forms unexamined;
- regresses idea 277 return-result behavior or idea 278 scalar local operand
  materialization;
- absorbs the `00004.c`/`00005.c` pointer/address-exposed local owner into this
  idea without trace evidence that it is the same branch-control bug.
