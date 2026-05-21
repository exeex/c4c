# AArch64 Prepared Select Condition Join Stale Reload

Status: Open
Created: 2026-05-21
Split From: ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md

## Goal

Repair the AArch64 prepared select or condition publication residual where a
false scan exit materializes an explicit zero/false value, but the join reloads
a stale stack slot and overwrites that false value.

## Why This Exists

Idea 362 repaired the pointer-derived load/address scaling timeout owner in
`00181`. After that repair, `00181` no longer times out: it advances to a fast
runtime nonzero segmentation fault.

The new first bad fact is a different owner in generated `Move`: the bounded
source scan false edge emits `mov x13, #0`, but the join reloads
`ldr x13, [sp, #64]`, replacing the false value with a stale stack slot. The
same select/join shape appears in the destination scan. This is not an address
scale multiply or pointer-derived load-address rule.

## In Scope

- Localize the prepared-BIR and generated-AArch64 boundary that decides whether
  a select/condition result must be published to its join-visible home.
- Repair the general prepared select or condition publication rule so explicit
  false/zero edges survive joins instead of being overwritten by stale stack
  reloads.
- Add focused backend coverage for a branch/join select or condition shape
  that would previously reload stale stack state over an explicit false value.
- Prove `00181` advances beyond the current fast segmentation fault or passes,
  while keeping the repaired pointer-derived address scaling evidence stable.

## Out Of Scope

- Reopening pointer-derived load/address scaling from idea 362 except to keep
  it stable.
- Reopening materialized pointer-addressed store writeback from idea 361 except
  to keep it stable.
- Reopening the idea 360 direct `LoadGlobal` current-memory select-store
  repair except to keep it stable.
- Reopening recursive formal post-call repairs from ideas 357, 358, and 359.
- Semantic-BIR dynamic pointer-derived string loads for `00173`.
- Frontend or semantic admission failure for `00005`.
- ABI-wide composite/byval/HFA/f128 work, variadic floating work, dynamic
  stack work, unrelated load-address scaling residuals, expectation changes,
  unsupported classifications, runner behavior, timeout policy,
  CTest-registration, or proof-log policy.

## Acceptance Criteria

- The first bad fact is localized to a concrete prepared select/condition join
  publication boundary, including the false-edge producer, expected join-visible
  value, stale stack reload, and consuming path.
- Focused backend coverage fails before the repair and passes after it for a
  select/condition join publication shape independent of `00181`.
- Generated code for `Move` preserves the explicit false/zero scan-exit value
  across the source and destination scan joins.
- `00181` advances beyond the current fast segmentation fault or passes without
  expectation, runner, timeout, or filename-specific changes.
- The idea 362 pointer-derived address scaling repair remains stable, including
  distinct index and scale registers in the representative multiply shape.
- `00170`, `00189`, and focused backend contracts for the nearby memory and
  prepared-BIR shapes remain passing.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00181`, `Move`, Hanoi tower globals, one block label, one stack
  offset such as `[sp, #64]`, one ABI register, or one emitted instruction
  neighborhood instead of repairing a general prepared select/condition join
  publication rule;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, classification notes, emitted-text
  rewrites, or pass-count movement while the join can still reload stale stack
  state over an explicit false/zero edge value;
- reopens pointer-derived load/address scaling, materialized pointer store
  writeback, direct `LoadGlobal` select-store handling, recursive formal
  post-call repairs, semantic string loads, frontend admission, ABI composite
  work, or variadic/floating residuals without fresh first-bad-fact evidence
  and a lifecycle split;
- leaves the exact old failure mode hidden behind a renamed helper or new
  abstraction, where false scan exits still materialize zero and then lose it
  to a stale join reload.
