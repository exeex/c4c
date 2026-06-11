# 195 Cross-target route-view reuse beyond x86 Route 6

## Goal

Identify and prove additional x86 and riscv wrapper boundaries that can consume
already-proven AArch64 route views while preserving target-local policy.

## Why This Exists

Idea 189 proved one cross-target reuse point: x86 `ConsumedPlans` can reuse the
AArch64-proven Route 6 call-use source view when route and prepared call-plan
source ids agree. The pre-Phase-E readiness audit concludes that this is not
broad cross-target readiness and gives no riscv proof.

Future retirement analysis needs a disciplined map of which cross-target
wrappers can reuse route views and which facts must remain target-local.

## In Scope

- Inventory x86 and riscv wrapper boundaries that currently consume prepared
  route facts or prepared compatibility helpers.
- Identify candidates that can consume route views already proven on AArch64.
- Require fallback behavior when route facts are absent, mismatched, ambiguous,
  or policy-sensitive.
- Preserve target-local ABI, frame, register, formatting, wrapper, instruction
  selection, and emission policy.
- Define proof expectations for each candidate reuse boundary.

## Out Of Scope

- Inventing target-only BIR adapters before an AArch64 semantic route view is
  proven.
- Treating x86 Route 6 `ConsumedPlans` as evidence for all x86 or riscv route
  reuse.
- Moving target ABI, frame, register allocation, instruction selection, or
  final emission policy into route views.
- Rewriting target wrappers unrelated to a named route-view reuse boundary.
- Opening draft 155 or deleting cross-target prepared compatibility surfaces.

## Acceptance Criteria

- A durable cross-target table names x86 and riscv wrapper candidates,
  required route views, fallback behavior, and retained target policy.
- At least one proposed reuse boundary beyond x86 Route 6 has a clear proof
  route, or the idea explains why no additional boundary is ready yet.
- riscv status is explicit and not inferred from x86 behavior.
- The output distinguishes semantic route-view reuse from target-local policy
  migration.
- Future implementation ideas can be scoped to one wrapper boundary at a time.

## Completion Note

Closed with `docs/bir_prealloc_fusion/cross_target_route_view_reuse_map.md`.
The audit found no ready boundary beyond the existing x86 Route 6 scalar
helper, made riscv status explicit, and left future implementation scope as
one-wrapper-boundary candidates rather than broad route-view migration.

## Reviewer Reject Signals

- Inventing riscv-only or x86-only BIR adapters that bypass the AArch64-proven
  route-view ladder.
- Treating one x86 scalar call-use reuse as broad cross-target or route-wide
  readiness.
- Moving ABI, frame, register, wrapper, formatting, instruction selection, or
  emission decisions into shared route facts.
- Dropping prepared fallback for absent or mismatched cross-target route facts.
- Claiming progress through wrapper renames or expectation changes without a
  real semantic route-view reuse.
- Proving only a named wrapper case while nearby same-route target wrappers
  remain unexamined.
