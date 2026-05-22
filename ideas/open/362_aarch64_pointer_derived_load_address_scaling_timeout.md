# AArch64 Pointer-Derived Load Address Scaling Timeout

Status: Parked - scope satisfied; close deferred
Created: 2026-05-21
Split From: ideas/open/361_aarch64_materialized_pointer_storelocal_writeback.md

## Goal

Repair the AArch64 lowering residual where pointer-derived loads or address
scaling in `00181` can produce a runtime timeout after materialized
pointer-addressed stores are correctly written back.

## Why This Exists

Idea 361 repaired the store-writeback owner: generated `Move` assembly now
reloads computed pointers and emits real stores through them before
`PrintAll`. The old `00181` failure mode, repeated unchanged later Hanoi
states, is gone.

The next observed failure is a 5 second timeout. The suspicious evidence is on
the adjacent load/address side, including generated address scaling such as
`mov x9, #4; mul x9, x9, x9` around pointer-derived loads. That is a separate
owner from missing materialized pointer store writeback and needs its own
first-bad-fact localization.

## In Scope

- Localize the first pointer-derived load or address-scaling boundary in
  `00181` that can explain the post-writeback timeout.
- Compare semantic BIR, prepared BIR, and generated AArch64 for the producing
  pointer value, scale/index calculation, emitted load address, and loop or
  recursion consumer.
- Repair the general AArch64 lowering rule for pointer-derived load address
  scaling when the address is computed from a pointer value.
- Add focused backend coverage for the localized pointer-derived load/address
  scaling shape independent of `00181`.
- Prove `00181` advances beyond the timeout or passes while preserving the
  idea 360 starting-state repair and idea 361 materialized pointer store
  writeback.

## Out Of Scope

- Reopening materialized pointer-addressed store writeback from idea 361 except
  to keep it stable.
- Reopening the idea 360 direct `LoadGlobal` current-memory select-store
  repair except to keep it stable.
- Reopening stack-preserved pointer formal post-call overwrite handling from
  idea 359.
- Reopening scalar formal post-call reloads from idea 358.
- Reopening pointer-formal callee-saved home publication from idea 357.
- Semantic-BIR dynamic pointer-derived string loads for `00173`.
- Frontend or semantic admission failure for `00005`.
- ABI-wide composite/byval/HFA/f128 work, variadic floating work, dynamic
  stack work, unrelated scalar compare/select residuals, expectation changes,
  unsupported classifications, runner behavior, timeout policy,
  CTest-registration, or proof-log policy.

## Acceptance Criteria

- The first bad fact is localized to a concrete pointer-derived load or address
  scaling boundary, including the source load operation, pointer/index carrier,
  expected address, emitted address calculation, and timeout-causing consumer.
- Focused backend coverage fails before the repair and passes after it for the
  localized pointer-derived load/address scaling shape.
- `00181` advances beyond the 5 second timeout or passes without expectation,
  runner, timeout, or filename-specific changes.
- The idea 360 starting-state output remains correct.
- The idea 361 materialized pointer store writeback evidence remains present.
- `00170`, `00189`, and focused backend contracts for the repaired memory
  shapes remain passing.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00181`, `Hanoi`, a tower name, one source line, one stack
  offset, one ABI register, or one emitted instruction neighborhood instead of
  repairing a general pointer-derived load/address scaling rule;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, classification notes, emitted-text
  rewrites, or pass-count movement while the timeout still traces to the same
  pointer-derived load/address scaling failure;
- reopens materialized pointer store writeback, direct `LoadGlobal`
  select-store handling, recursive formal post-call repairs, semantic string
  loads, frontend admission, ABI composite work, or variadic/floating
  residuals without fresh first-bad-fact evidence and a lifecycle split;
- regresses the idea 360 starting-state output, idea 361 store writeback, or
  the already repaired ideas 357, 358, and 359 stability paths.

## Parked Lifecycle Note

2026-05-21: commit `321031ce0` repaired the pointer-derived load/address
scaling timeout owner by preserving the live index/result carrier when
materializing immediate scales. Focused proof showed `00181` advanced from the
5 second timeout to a fast runtime nonzero, with `00170`, `00189`, and the
focused backend contracts passing. The broader backend guard reported 141/141
passing.

The new first bad fact is not pointer-derived load/address scaling: false scan
exits materialize `mov x13, #0`, but the join reloads stale stack slots such
as `ldr x13, [sp, #64]`, overwriting the false value. That residual was split
to `ideas/open/363_aarch64_prepared_select_condition_join_stale_reload.md`.

Source intent for this idea is satisfied, but close was rejected by the
close-time monotonic regression gate because the available focused before/after
logs classify `00181` as a changed failing test and the full baseline candidate
introduced different failing tests despite a lower total failure count. Keep
this idea parked until the supervisor can provide an acceptance-grade guard
that permits archival closure.

2026-05-22: reactivation commits `b31ef2e53` and `10b03efd4` refreshed the
current tree. Step 1 focused proof was 7/7 green for the memory operand
contracts, materialized pointer writeback guard, and AArch64 `00170`, `00181`,
and `00189`. Current `00181` generated address calculations use `sxtw` plus
`lsl #2` and no `mul` occurrence matching the old suspicious immediate-scale
pattern. The pointer-derived load/address-scaling timeout owner is still not
live, so the active refresh runbook was deactivated. Leave this source idea
parked rather than closed until an acceptance-grade matching regression guard
is available for archival closure.
