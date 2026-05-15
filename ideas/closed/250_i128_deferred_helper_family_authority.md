# I128 Deferred Helper Family Authority

Status: Closed
Created: 2026-05-15
Closed: 2026-05-15

Parent Context: ideas/closed/236_aarch64_i128_pair_lowering.md

## Goal

Define prepared/shared authority for i128 helper families that were explicitly
deferred from the supported AArch64 i128 pair-lowering route: float/i128
conversion helpers and helper families that require memory-return ownership.

## Why This Exists

Idea 236 closed the supported i128 pair-lowering route for direct register-pair
transport, arithmetic, bitwise, shifts, comparisons, and direct-result div/rem
helper-call printing. It intentionally kept float/i128 conversion helpers and
memory-return helper families out of scope because they require additional
source-operation mapping, destination ownership, storage extent, ABI binding,
marshaling, live-preservation, and selected-call facts before target lowering
or printing can consume them.

## In Scope

- Define source-operation mapping for supported i128 float-conversion helper
  families.
- Define helper kind, callee, argument/result ownership, ABI, register-bank,
  clobber/resource, and marshaling facts for those conversion helpers.
- Define memory-return helper ownership facts when a helper returns through
  memory: destination identity, storage extent, alignment, slot, offset, and
  live-preservation requirements.
- Preserve fail-closed diagnostics for unsupported helper families,
  incomplete memory-return ownership, incomplete ABI bindings, and incomplete
  live-preservation state.
- Make the facts consumable by a later AArch64 selected-record and printer
  route without hard-coded helper ABI registers.

## Out Of Scope

- Do not reopen direct-result div/rem helper-call printing already covered by
  idea 236 unless a regression or missing shared fact is found.
- Do not lower helper-required operations as scalar i64 shortcuts.
- Do not hard-code helper ABI registers, fixed low/high lane pairs, register
  adjacency, rendered names, or helper callee strings in AArch64 target
  lowering.
- Do not broaden into binary128 soft-float, scalar FP, atomics, intrinsics,
  inline asm, generic call lowering, callee-save placement, or preserved-value
  extent work.

## Acceptance Criteria

- Supported deferred i128 helper families expose complete prepared/shared
  source-operation, helper, lane/result, ABI, marshaling, clobber/resource, and
  live-preservation facts.
- Memory-return helper families expose explicit destination ownership and
  storage extent facts, or fail closed with diagnostics.
- A later AArch64 consumer can select and print supported helper boundaries
  from structured facts without target-local helper synthesis or fixed-register
  assumptions.
- Existing supported div/rem helper-call behavior remains stable.

## Reviewer Reject Signals

- The route hard-codes helper ABI registers, callee names, or lane ownership in
  AArch64 target lowering instead of prepared/shared facts.
- The route claims support through expectation rewrites while missing
  source-operation, memory-return, ABI binding, marshaling, clobber/resource,
  or live-preservation facts remain absent.
- The route lowers i128 helper-required operations as scalar i64 or named-case
  shortcuts.
- The route broadens into binary128, atomics, intrinsics, inline asm, generic
  call lowering, callee-save placement, or unrelated frame allocation work.

## Closure Summary

The active runbook completed the prepared/shared authority for the supported
direct-result F32/F64 i128 conversion helper family:
`__fixsfti`, `__fixdfti`, `__fixunssfti`, `__fixunsdfti`,
`__floattisf`, `__floattidf`, `__floatuntisf`, and `__floatuntidf`.
Those helpers now expose structural source-operation mapping, helper
family/kind, callee identity, direct result and argument lane ownership,
ABI/register-bank bindings, marshal/unmarshal moves, runtime-helper resources,
caller-saved clobber policy, live-preservation evaluation, and selected-call
ownership.

F128/binary128 conversion mappings remain intentionally deferred by the
idea's out-of-scope boundary. I128 helper memory-return shapes remain
fail-closed because destination identity, storage extent/alignment, slot,
offset, producer authority, and helper ABI policy are not complete for
helper-selected memory results.

Close-time regression guard used matching full-suite logs:
`test_before.log` and `test_after.log` both reported 3167/3167 passing tests
with no new failures.
