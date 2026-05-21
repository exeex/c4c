# AArch64 Block Label Emission Ordering

Status: Open
Created: 2026-05-21
Split From: ideas/open/349_aarch64_recursive_call_argument_preservation.md

## Goal

Repair AArch64 function/block emission so generated labels, fallthrough
boundaries, and return/epilogue placement preserve prepared basic-block
control-flow order instead of allowing later executable code to appear after a
return block without a reachable label.

## Why This Exists

Idea 349 advanced `00176` past stale recursive call-argument publication from
caller-clobbered registers. The new first bad fact is in generated
`partition` block ordering, not call preservation: prepared metadata reports
`block_3` as a return block followed by `block_4` and `block_5`, but generated
assembly prints the `block_3` return/epilogue before later `swap` code, and
that later code has no visible block label before it.

Continuing this under call-preservation would widen idea 349 into block
layout and label emission. The new owner should establish whether the
selected AArch64 block order, label placement, fallthrough handling, or
epilogue emission is violating prepared CFG semantics.

## In Scope

- Localize how prepared basic blocks are ordered and emitted for AArch64
  functions with a return block followed by later reachable blocks.
- Repair label placement, block emission ordering, fallthrough boundaries, or
  return/epilogue placement when generated assembly no longer matches the
  prepared CFG.
- Cover a focused multi-block AArch64 shape where a return block and later
  reachable block sequence must not collapse into unlabeled fallthrough after
  an epilogue.
- Use `00176` `partition` only as external proof after focused backend
  coverage identifies the shared emission owner.

## Out Of Scope

- Recursive call argument preservation, preserved-home publication, caller-save
  reloads, or stale argument-register fixes already handled by idea 349.
- Stack-preserved symbol/local publication crashes such as the current `00181`
  first bad fact.
- Indexed aggregate address/writeback, variadic/byval publication, scalar cast
  publication, fixed-formal entry publication, frame-slot layout, runner
  behavior, timeout policy, proof-log behavior, CTest registration, expectation
  changes, or unsupported-classification changes.
- Fixing only `00176`, `partition`, one block number, one label spelling, one
  emitted return sequence, or one generated instruction neighborhood.

## Acceptance Criteria

- The first bad fact is localized to a concrete AArch64 block-ordering,
  label-placement, fallthrough, branch-target, or epilogue-emission boundary.
- Focused backend coverage fails without the repair and passes with it for a
  multi-block function where a return block is followed by later reachable
  blocks in prepared order.
- `00176` advances past the current unlabeled post-epilogue `partition` code
  failure or is reclassified by a new first bad fact.
- Adjacent AArch64 branch, return, call-publication, and selected-address
  guardrails selected by the supervisor remain stable.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00176`, `partition`, one block id, one label suffix, one
  return sequence, one branch target, or one emitted instruction neighborhood
  instead of repairing general AArch64 block emission semantics;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, diagnostic rewrites, comment-only
  notes, or generated-text reshuffling that still leaves reachable code after
  a return/epilogue without a correct label and branch path;
- broadens back into call-preservation, indexed aggregate writeback,
  stack/local publication, frame layout, or variadic/byval publication without
  fresh first-bad-fact evidence;
- proves only the current `partition` snippet while nearby multi-block return,
  branch, and fallthrough shapes remain unexamined.
