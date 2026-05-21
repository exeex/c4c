# AArch64 Signed Remainder Lowering

Status: Open
Created: 2026-05-21
Split From: ideas/open/364_aarch64_synthetic_select_label_uniqueness.md

## Goal

Repair the AArch64 signed integer remainder lowering residual where `%`
materialization can multiply the quotient by itself instead of by the divisor,
producing an incorrect switch selector or scalar result.

## Why This Exists

Idea 364 repaired the duplicate synthetic select-label assembler failure in
`00143`. The representative now reaches runtime and fails because Duff's
device dispatch uses `switch (count % 8)` with `count == 39`, where the
expected selector is `7`.

Generated AArch64 computes the surrounding `(count + 7) / 8` path correctly,
but the later signed remainder lowering emits `sdiv w9, w13, w9` followed by
`msub w13, w9, w9, w13`. That computes `39 - 4 * 4 == 23` instead of
`39 - 4 * 8 == 7`, so the switch misses cases 0 through 7 and the copy loop
does not populate `b`.

This is not the synthetic label owner from idea 364 and should not be silently
folded into the parked unsigned div/rem producer-publication idea 350, whose
scope explicitly excludes signed division/remainder lowering.

## In Scope

- Localize signed integer remainder lowering for AArch64 scalar values,
  including the dividend, divisor, quotient, and `msub` operand assignment.
- Repair signed remainder materialization so `a % b` computes
  `a - (a / b) * b` with the original divisor, not the quotient.
- Cover signed remainder feeding switch selection or ordinary scalar
  consumers without hard-coding `00143`, `count`, `8`, one register, or one
  emitted instruction sequence.
- Prove `c_testsuite_aarch64_backend_src_00143_c` advances past the bad
  `count % 8` selector or passes.

## Out Of Scope

- Unsigned div/rem producer publication from idea 350 unless fresh evidence
  proves the same signed repair boundary also owns an unsigned failure.
- Synthetic select/materialized-label uniqueness from idea 364.
- Basic block label ordering, scalar-cast source publication, indexed
  aggregate writeback, pointer comparisons, scalar FP arithmetic, file I/O
  call-result publication, aggregate initializer layout, enum bit-field
  layout, or timeout cases from the backend inventory.
- Runtime correctness after `00143` advances beyond the signed remainder first
  bad fact.
- Changing expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or test contracts.

## Acceptance Criteria

- The first bad fact is localized to a concrete signed remainder lowering,
  operand-selection, or publication boundary.
- Focused backend coverage proves signed remainder uses the original divisor
  when forming the multiply-subtract result.
- The repair is general for signed integer remainder and not a named
  `00143`/Duff's-device shortcut.
- `c_testsuite_aarch64_backend_src_00143_c` no longer fails from
  `count % 8` producing selector `23` instead of `7`.
- Any remaining `00143` failure is reclassified by its new first bad fact
  before this idea is closed.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00143`, Duff's device, `count`, literal `8`, one source line,
  one register pair, or the exact `sdiv`/`msub` text instead of repairing
  signed remainder lowering generally;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress by changing switch dispatch, labels, fallthrough ordering,
  or test output while signed `%` can still use the quotient as the `msub`
  multiplier operand;
- broadens into unsigned div/rem, synthetic label uniqueness, aggregate
  writeback, scalar FP, pointer comparisons, or other backend inventory
  buckets without fresh first-bad-fact evidence and lifecycle routing;
- proves only the external representative while leaving focused signed
  remainder behavior unguarded.
