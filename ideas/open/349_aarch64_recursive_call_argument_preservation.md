# AArch64 Recursive Call Argument Preservation

Status: Open
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
