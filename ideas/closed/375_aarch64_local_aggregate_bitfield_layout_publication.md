# AArch64 Local Aggregate Bit-Field Layout Publication

Status: Closed
Created: 2026-05-21
Closed: 2026-05-21
Split From: ideas/closed/374_aarch64_local_aggregate_address_call_publication.md

## Closure Note

Closed after focused local aggregate bit-field layout publication coverage and
the `00218` representative both passed. The old mismatch between the local
store offset and the scalar pointer consumer load offset is gone, no new first
bad fact was recorded for `00218`, and matching canonical `^backend_`
before/after logs reported 144/144 backend tests passing with the monotonic
regression guard passing.

## Goal

Repair AArch64 lowering where a local aggregate field or bit-field store is
published at a frame offset that disagrees with the layout offset later
consumed through a scalar pointer argument.

## Why This Exists

Idea 374 repaired stale local aggregate address call publication. Its proof
advanced `00218`: `main` now materializes `&convs` before calling
`convert_like_real`, so the old stale-pointer first bad fact is gone.

The new `00218` first bad fact is layout/store publication. Generated `main`
stores `AMBIG_CONV` near `sp+2`, while `convert_like_real` reads and masks the
code field from `[x0,#16]`. That mismatch is not a call-argument address
publication failure; it is a local aggregate layout, bit-field store, or
frame-offset publication problem.

`00216` also advanced past its first local aggregate `print(...)` address
calls. Its current visible residual appears later, around
compound-relocation/function-pointer-table lowering, so it is an adjacency
guard only for this idea unless fresh evidence ties it to the same aggregate
layout/store boundary.

## In Scope

- Localize the mismatch between local aggregate layout metadata and generated
  AArch64 stores for bit-field or small-field members.
- Trace source aggregate layout, prepared offsets, frame homes, and emitted
  stores for `00218`.
- Repair the general layout/store publication boundary so local aggregate
  field writes agree with scalar pointer consumer reads.
- Add focused backend coverage for local aggregate bit-field or small-field
  layout publication.
- Prove `c_testsuite_aarch64_backend_src_00218_c` advances past the
  `AMBIG_CONV` store/load offset mismatch or passes.

## Out Of Scope

- Local aggregate address call publication, which is closed by idea 374.
- `00216` compound-relocation or selected function-pointer-table lowering
  unless localization proves the same aggregate layout/store owner.
- Scalar constant/`sizeof` stack-home publication such as `00205`.
- External/libc call-result publication such as `00187`.
- Scalar FP expression/constant materialization such as `00174`.
- Dynamic stack/VLA fixed-slot timeout work such as `00207`.
- Shift/type-promotion timeout work such as `00200`.
- Fixing only `00218`, `convs`, `convert_like_real`, `AMBIG_CONV`, one field
  name, one stack offset, one register, or one emitted instruction sequence.

## Acceptance Criteria

- The current first bad fact is localized to a concrete aggregate layout,
  bit-field store, prepared offset, frame-slot, MIR handoff, or AArch64 store
  publication boundary.
- Focused backend coverage guards local aggregate bit-field or small-field
  layout publication without relying only on `00218`.
- `c_testsuite_aarch64_backend_src_00218_c` no longer fails because
  `AMBIG_CONV` is stored at an offset inconsistent with the callee's layout
  load.
- Any `00216` observation is kept to classification unless it shares the same
  localized aggregate layout/store boundary.
- The supervisor-selected proof scope introduces no new backend-regex
  failures and does not regress recent pointer-local, selected-value, scalar
  publication, direct-call, or local aggregate address publication guardrails.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00218`, `convs`, `convert_like_real`, `AMBIG_CONV`, one
  field name, one stack offset, one register, or one emitted instruction
  sequence instead of repairing local aggregate layout/store publication
  generally;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, emitted-text reshuffling, or
  classification-only notes while local aggregate field stores still disagree
  with scalar pointer consumer layout offsets;
- reopens idea 374's local aggregate address call publication boundary without
  fresh generated-code evidence that contradicts its closure;
- folds `00216` relocation/function-pointer-table lowering, scalar constants,
  external call-result publication, scalar FP, dynamic-stack timeout, or
  shift-promotion timeout work into this route without a fresh first-bad-fact
  handoff;
- proves only the external representative while leaving focused local
  aggregate bit-field or small-field layout publication behavior unguarded.
