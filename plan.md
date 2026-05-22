# AArch64 Recursive Stack-Preserved Pointer Formal Post-Call Overwrite Runbook

Status: Active
Source Idea: ideas/open/359_aarch64_recursive_stack_preserved_pointer_formal_post_call_overwrite.md
Activated from: parked source idea after closing ideas/closed/358_aarch64_recursive_scalar_formal_post_call_preservation.md

## Purpose

Refresh and resolve the parked stack-preserved pointer-formal post-call owner
that was split from idea 358.

## Goal

Confirm whether the `%p.spare`-style stack-preserved pointer formal
post-call overwrite first bad fact is still live, then either close the source
idea or hand off a freshly localized residual without widening the owner.

## Core Rule

Do not repair a new `00181` failure under this idea unless fresh evidence shows
the first bad fact is a stack-preserved pointer formal being overwritten or
reloaded from a clobbered ABI argument register after a call.

## Read First

- `ideas/open/359_aarch64_recursive_stack_preserved_pointer_formal_post_call_overwrite.md`
- `ideas/closed/358_aarch64_recursive_scalar_formal_post_call_preservation.md`
- Existing focused backend coverage for ideas 357, 358, and 359.
- Generated AArch64 for `c_testsuite_aarch64_backend_src_00181_c` only as a
  representative, not as a testcase-shaped repair target.

## Current Targets

- `00181` recursive `Hanoi` / `Move` path around stack-preserved pointer
  formals such as `%p.spare`.
- Focused backend guardrails that prove:
  - scalar formal post-call reloads from idea 358 stay repaired;
  - pointer-formal callee-saved home publication from idea 357 stays repaired;
  - stack-preserved pointer formal homes are not republished from clobbered ABI
    argument registers after calls.

## Non-Goals

- Do not reopen scalar `%p.n` post-call preservation from idea 358.
- Do not reopen callee-saved pointer-formal home publication from idea 357.
- Do not reopen Hanoi starting-state, materialized pointer store writeback,
  pointer-derived load/address scaling, address-valued publication, semantic
  string loads, frontend admission, ABI composite, variadic, or floating work
  without a lifecycle split.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log policy.
- Do not special-case `00181`, `Hanoi`, `Move`, `%p.spare`, one stack offset,
  one ABI register, or one emitted instruction neighborhood.

## Working Model

The parked source idea records that commit `34387ce97` repaired the
stack-preserved pointer formal post-call overwrite: generated `Hanoi` no
longer emits the bad post-call `str x3, [sp, #8]` between `bl Move` and the
final reload/call sequence. The prior residual moved outside this owner and
was split to later ideas. After idea 358 was closed, this idea needs a fresh
current-tree check to determine whether it can also be closed or whether a
new, in-scope first bad fact has returned.

## Execution Rules

- Treat source-idea scope as the contract; do not expand this runbook to chase
  unrelated current failures.
- Prefer read-only generated artifact inspection before code edits.
- If the old overwrite pattern is absent and representative/focused proof is
  green or non-decreasing under the accepted lifecycle close mode, request
  plan-owner closure rather than implementation work.
- If `00181` fails for a different first bad fact, write the handoff at the
  lifecycle layer and split or activate the appropriate owner instead of
  mutating this idea.
- For any code-changing step, require build proof plus the supervisor-selected
  focused CTest subset before acceptance.

## Step 1: Refresh Stack-Preserved Pointer-Formal First Bad Fact

Goal: Determine whether the old `%p.spare` post-call overwrite owner is still
live on the current tree.

Primary Target: generated AArch64 and focused backend tests for
`c_testsuite_aarch64_backend_src_00181_c`, plus nearby idea 357 and 358
guardrails.

Actions:

- Build the current tree.
- Run the focused supervisor-selected subset for `00181`, `00170`, `00189`,
  and backend contracts covering recursive formal preservation.
- Inspect generated `00181` assembly around `Hanoi` / `Move` if the
  representative fails or if proof output indicates a regression.
- Check specifically for post-call stores that republish a live
  stack-preserved pointer formal home from a clobbered ABI argument register.

Completion Check:

- The old stack-preserved pointer-formal overwrite is either confirmed absent
  with focused proof, or localized as the first bad fact with concrete
  generated evidence.

## Step 2: Close Or Reclassify

Goal: Resolve the lifecycle state for idea 359 based on Step 1 evidence.

Primary Target: `plan.md`, `todo.md`, and
`ideas/open/359_aarch64_recursive_stack_preserved_pointer_formal_post_call_overwrite.md`.

Actions:

- If Step 1 shows no current in-scope first bad fact, request plan-owner close
  using matching focused before/after logs or the accepted non-decreasing
  maintenance close mode used for idea 358.
- If Step 1 shows a returned in-scope first bad fact, keep this idea active and
  delegate a bounded implementation packet for the localized owner.
- If Step 1 shows a different first bad fact, record the handoff at the
  lifecycle layer and split or activate the appropriate separate source idea.

Completion Check:

- Idea 359 is closed, or the active lifecycle points to an explicitly
  localized in-scope implementation packet, or the out-of-scope residual is
  handed off without expanding this source idea.
