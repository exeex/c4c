# AArch64 Local/Formal Frame-Slot Publication

Status: Parked
Created: 2026-05-21
Split From: ideas/open/352_aarch64_block_label_emission_ordering.md

## Goal

Repair AArch64 scalar local/formal publication so incoming formal values are
published into the local frame slots that later local loads consume, before
those local slots are read.

## Why This Exists

Idea 352 repaired the block label/emission ordering failure exposed by `00176`
`partition`: generated assembly now preserves the `.LBB90_6`/`.LBB90_7`
branch/label path instead of placing later reachable code after a return
epilogue without a reachable label.

The remaining `00176` segfault is a different first bad fact. On entry to
`partition(0, 15)`, debugger evidence showed `w0=0` and `w1=15`, but generated
code read stale stack contents from `[sp]` and propagated that value into local
slots used for `pivotIndex`, `index`, and `i`. The resulting bogus partition
index drives invalid recursive quicksort bounds and eventual stack exhaustion.

This should be handled as local/formal frame-slot publication, not as another
block-label or branch-emission repair.

## In Scope

- Localize where prepared AArch64 values for incoming scalar formals are meant
  to become available to local stack slots consumed by `load_local` or
  `store_local` lowering.
- Repair the semantic publication path from register homes such as `x0`/`w0`
  and `x1`/`w1` into local frame slots when a function initializes locals from
  fixed scalar formals.
- Add or extend focused backend coverage for a function that stores incoming
  scalar formals into locals, reads those locals before and after a call, and
  returns a local-derived value.
- Use `00176` `partition` as external representative proof after focused
  backend coverage identifies the shared publication owner.

## Out Of Scope

- AArch64 block label placement, block ordering, fallthrough handling, and
  return/epilogue placement already handled by idea 352.
- Recursive call argument preservation, preserved-home publication,
  caller-save reloads, or stale argument-register repairs handled by idea 349.
- Variadic, byval, HFA, aggregate argument, indexed aggregate address/writeback,
  scalar cast stack-home, or selected-address publication work unless fresh
  evidence proves the same owner is required for scalar fixed formals.
- Broad frame-layout rewrites, CTest registration, runner behavior, timeout
  policy, proof-log behavior, expectation changes, or unsupported-classification
  changes.
- Fixing only `00176`, `partition`, one local name, one stack offset, one
  formal register number, or one generated instruction neighborhood.

## Acceptance Criteria

- The first bad fact is localized to a concrete AArch64 publication boundary
  for scalar fixed formals feeding local frame slots.
- Focused backend coverage fails without the repair and passes with it for a
  function that initializes locals from incoming scalar formals, reads those
  locals, crosses at least one call boundary, and returns a local-derived value.
- `00176` advances past the bogus `partition` local/formal slot initialization
  failure or is reclassified by a new first bad fact.
- Adjacent AArch64 local load/store, call argument publication, branch/control,
  return, selected-address, and frame-slot guardrails selected by the supervisor
  remain stable.

## Lifecycle Handoff

2026-05-21: The active repair published incoming register-backed fixed formals
into the local frame slots consumed by generated AArch64 local loads. Focused
proof stayed 5/6, but `00176` advanced from timeout/stack exhaustion to a fast
runtime output mismatch after the stale `partition` formal/local-slot reads
were repaired. Supervisor broader backend guard for the repair passed 141/141.

The remaining `00176` first bad fact is outside this idea: generated `swap`
appears to write global indexed array elements from uninitialized high stack
snapshot slots such as `[sp, #264]` and `[sp, #268]`. That residual belongs to
indexed aggregate/global array selected-address writeback work, so the active
lifecycle switches to `ideas/open/348_aarch64_indexed_aggregate_address_writeback.md`.

Close is deferred instead of moving this file to `ideas/closed/` because the
available canonical narrow regression logs still report 5/6 before and after,
and the guard script classifies the timeout-to-runtime-mismatch transition as a
new failing test rather than a close-accepted monotonic pass increase. Resume
this idea only if fresh evidence again shows scalar fixed formal-to-local
frame-slot publication is the first bad fact.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00176`, `partition`, `pivotIndex`, `index`, `i`, one stack
  offset, one formal register, one function name, or one emitted instruction
  neighborhood instead of repairing scalar formal-to-local publication;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, diagnostic rewrites, comment-only
  notes, or generated-text reshuffling that still leaves local slots reading
  stale stack data before formal values are published;
- broadens back into block label emission, recursive call preservation,
  variadic/byval/HFA handling, indexed aggregate writeback, or broad frame
  layout without fresh first-bad-fact evidence;
- proves only the current `partition` snippet while nearby scalar formal to
  local load/store and call-crossing shapes remain unexamined.
