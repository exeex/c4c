# 119 AArch64 Calls Before-Call Move Bundle Local Owner

## Goal

Extract a bounded AArch64-local owner for before-call move bundle lowering that
consumes already-prepared call, move, source, ABI, immediate, f128, stack-slot,
and outgoing-stack-area facts without rediscovering them.

## Why This Exists

Idea 118 classified before-call move bundle lowering as `ready-local-owner`.
The current calls lowering has enough prepared authority to separate the
translation from prepared move facts into AArch64 machine records from the
larger `calls.cpp` flow. The owner boundary is local emission glue, not a new
shared call-planning contract.

## In Scope

- AArch64-local before-call move emission around `lower_before_call_move`.
- Helper surfaces that translate prepared authority into register, memory,
  immediate, FP, and f128 call-boundary machine records.
- Consumption of `PreparedCallPlan`, `PreparedMoveBundle`,
  `PreparedCallBoundaryEffectPlan`, `PreparedMoveResolution`, prepared value
  homes, prepared ABI bindings, prepared source selections, prepared f128
  carriers, indexed immediate argument lookups, and
  `PreparedCallPlan::outgoing_stack_argument_area`.
- Preservation of AArch64-owned details such as `x16` outgoing stack base
  addressing, q-register rendering, inline-asm materialization, and target
  diagnostics for incomplete prepared authority.

## Out Of Scope

- Reselecting stack homes or reopening idea 93 stack/frame-slot operand
  authority.
- Reopening idea 94 f128 carrier lookup or q-register transport authority.
- Reopening idea 95 immediate scalar argument publication authority.
- Recomputing outgoing stack area totals covered by idea 114.
- Moving AArch64 ABI spelling, scratch registers, or machine-record emission
  into shared BIR/prealloc.
- A monolithic `calls.cpp` shrink unrelated to the before-call move bundle
  owner boundary.

## Likely Files

- `src/backend/mir/aarch64/codegen/calls.cpp`
- A new or existing AArch64 calls helper file if the local owner is split from
  `calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp` or nearby local declarations, if
  needed
- Focused backend/AArch64 tests that already cover before-call stack, register,
  immediate, FP, and f128 argument moves

## Owner Boundary

Prepared/shared code owns call planning, source selection, destination homes,
ABI bindings, immediate publication, f128 carrier facts, stack-frame-slot
facts, and outgoing stack area authority.

The new AArch64-local owner owns only the conversion from those prepared facts
into target call-boundary move records and machine instructions for before-call
argument movement.

## Proof Route

- Characterize the existing `lower_before_call_move` helper cluster before
  moving code.
- Run a narrow backend/AArch64 subset covering register arguments, stack
  arguments, immediate arguments, FP/f128 arguments, and outgoing stack area
  reservation.
- Include at least one route where the prepared outgoing stack area is consumed
  as the reservation/address authority while `x16` base emission stays
  AArch64-local.
- Use broader backend proof if shared prepared interfaces are touched.

## Acceptance And Completion Criteria

- Before-call move bundle lowering is owned by a bounded AArch64-local helper
  or file with no new shared authority.
- The implementation consumes closed-route prepared facts from ideas 93, 94,
  95, and 114 instead of duplicating them.
- AArch64-specific register spelling, q-register/f128 rendering, `x16`
  addressing, inline-asm materialization, and target diagnostics remain local.
- Focused proof covers nearby before-call argument shapes, not only one named
  testcase.

## Reviewer Reject Signals

- The route reselects stack homes, stack destinations, or prior stack-preserved
  sources instead of consuming idea 93 ownership.
- The route adds a new f128 carrier lookup, q-register transport authority, or
  module fallback path instead of consuming idea 94 ownership.
- The route rediscovers same-block immediate casts or immediate publication
  facts instead of consuming idea 95 ownership.
- The route recomputes outgoing stack area totals from per-argument
  destinations instead of consuming idea 114's call-level area.
- Tests are weakened, supported paths are marked unsupported, or expectations
  are rewritten to avoid proving before-call movement.
- The proof covers only one narrow fixture while nearby register, stack,
  immediate, FP, or f128 before-call paths remain unexamined.
- The diff claims progress through line-count reduction, helper renames, or a
  broad file split while the old before-call authority remains in place.
