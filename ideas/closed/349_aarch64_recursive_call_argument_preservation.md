# AArch64 Recursive Call Argument Preservation

Status: Closed
Created: 2026-05-20
Split From: ideas/open/348_aarch64_indexed_aggregate_address_writeback.md

## Goal

Repair AArch64 generated-code handling for live recursive-call arguments so a
caller does not reuse caller-clobbered argument registers after a nested or
recursive `bl` when later control-flow or calls still need the original
values.

## Why This Exists

Idea 348 repaired selected aggregate address/writeback representatives
`00130`, `00187`, and `00195`. The remaining red representatives `00176` and
`00181` now show a different first bad fact: generated recursive call
sequences reuse argument registers such as `w0` and `w1` after a call that may
clobber them.

For `00176`, generated quicksort/partition code still has global indexed swap
symptoms, but the current recursive path loses live low/high arguments across
recursive calls before later use. For `00181`, generated Tower of Hanoi code
segfaults with the same caller-clobbered argument reuse shape around recursive
calls. Continuing those failures under indexed aggregate writeback would widen
idea 348 into call-boundary preservation.

## In Scope

- Localize live values that must survive recursive or nested calls in AArch64
  generated code.
- Repair the handoff from prepared live arguments or locals to post-call uses
  when the value currently remains only in caller-clobbered call registers.
- Cover recursive ordinary calls such as quicksort and Tower of Hanoi shapes
  where later control-flow, array mutation, or another call consumes the
  original argument value.
- Add focused backend coverage that proves live caller-side values are
  preserved or reloaded across a nested `bl` before post-call use.

## Out Of Scope

- Dynamic indexed aggregate selected-address/writeback repairs already owned
  by idea 348.
- Variadic aggregate `va_arg`, byval aggregate lane publication, HFA/floating
  aggregate publication, fixed-formal entry publication, scalar cast
  publication, return publication, local conversion publication, or frame-slot
  layout work unless fresh generated-code evidence reaches those boundaries.
- Expectation changes, unsupported-classification changes, runner behavior,
  timeout policy, CTest registration, or proof-log behavior.
- Fixing only `00176`, only `00181`, one source function, one emitted register
  pair, or one instruction sequence.

## Acceptance Criteria

- The first bad fact is localized to a concrete live-value preservation,
  post-call reload, caller-save spill, or call-boundary handoff point.
- Focused backend coverage fails without the repair and passes with it for a
  recursive or nested-call value that is used after a `bl`.
- At least one of `00176` or `00181` advances past the caller-clobbered
  recursive argument reuse failure, and the other is either advanced or
  reclassified by a new first bad fact.
- Adjacent call publication and aggregate selected-address guardrails selected
  by the supervisor remain stable.

## Lifecycle Handoff

2026-05-21: Step 3/4 follow-up repaired stale cross-block call-argument
publication from callee-saved preservation homes. `00176` now emits
`mov x0, x20` before the final `bl swap` in `partition` block `block_3`, and
the stale `mov w0, w1` publication no longer appears in the generated file.
Focused proof remained 4/6 because `00176` and `00181` still segfault, but
the remaining failures are no longer the caller-clobbered recursive argument
reuse boundary owned by this idea:

- `00176` advanced to generated AArch64 block label/emission ordering after
  the `partition` `block_3` return. Prepared metadata has `block_3` as a
  return block followed by `block_4` and `block_5`, but generated assembly
  prints the `block_3` return/epilogue before later unlabeled `swap` code.
  That first bad fact is split to
  `ideas/open/352_aarch64_block_label_emission_ordering.md`.
- `00181` remains at the previously classified out-of-scope
  stack-preserved symbol/local publication crash. Later `Hanoi`
  call-preservation hazards may still exist, but they are not the current
  first bad fact until that earlier publication crash is fixed or bypassed by
  its owner.

Supervisor broader backend guard passed 141/141. Close was not accepted in
this lifecycle pass because the available canonical focused before/after logs
match at 4/6 and the strict regression guard rejects closure without a pass
count increase. Keep 349 parked as a completion candidate rather than active;
resume it only if fresh generated-code evidence again shows an in-scope stale
caller-clobbered post-call argument use.

2026-05-22: Reactivated only to refresh the representative first bad fact.
The focused command
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c)$'`
passed 2/2 for `00176` and `00181`, leaving no current representative failure
owned by this source idea. Close was still rejected because the close-time
monotonic regression guard compared matching green logs with
`passed=2 failed=0 total=2` before and after and requires a strict pass-count
increase. Keep this idea parked, not active, until fresh generated-code
evidence again shows an in-scope recursive or nested-call argument preservation
failure.

2026-05-23: Step 3 lifecycle decision accepted closure after the focused
refresh proof again found no live in-scope recursive or nested-call argument
preservation failure. The supervisor-provided focused proof command passed
2/2, and the plan-owner close gate used the rolled-forward focused log in
non-decreasing mode:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed`.
The guard reported before 2/2, after 2/2, delta 0, and no new failing tests.
No implementation work remains for this source idea unless fresh generated-code
evidence later reintroduces this exact preservation failure family.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00176`, `00181`, `quicksort`, `Hanoi`, one argument index,
  one register name, or one emitted `bl` neighborhood instead of repairing
  general live-value preservation across caller-clobbering calls;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, diagnostic rewrites, classification
  notes, or output-count changes without fixing generated post-call value
  consumption;
- broadens into indexed aggregate address/writeback, variadic/byval
  publication, frame-layout, or scalar ALU producer publication without fresh
  first-bad-fact evidence;
- leaves generated code able to consume a stale caller-clobbered argument
  register after a nested or recursive call when the source value is still
  live.
