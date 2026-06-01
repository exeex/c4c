# AArch64 Local Helper Duplication Tail Cleanup

## Goal

Finish the AArch64-local helper duplication tail called out by idea 74 by
folding repeated helper shapes in globals, atomics, cast, and memory owners into
existing local utilities or narrower owner-local helpers.

## Why This Exists

Idea 74 removed owned duplicate scalar register-view and compare-branch helper
logic, but its closure note explicitly left same-shaped helper duplication in
`globals`, `atomics`, `cast`, and `memory` for a separate source idea. These
helpers are target-local consumer utilities, not a reason to create new BIR
authority.

## Owned Files

- `src/backend/mir/aarch64/codegen/globals.cpp`
- `src/backend/mir/aarch64/codegen/globals.hpp`
- `src/backend/mir/aarch64/codegen/atomics.cpp`
- `src/backend/mir/aarch64/codegen/atomics.hpp`
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `src/backend/mir/aarch64/codegen/cast_ops.hpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- Existing local utility owners only if they already own the abstraction:
  `operands.*`, `prepared_value_home_materialization.*`, or
  `fp_value_materialization.*`.

## In Scope

- Inventory repeated helpers for register views, prepared value IDs, storage
  plan lookup, frame-slot lookup, diagnostics, prepared-record error wrapping,
  and occupied-register/scratch selection.
- Fold helpers into existing AArch64-local utilities when the abstraction is
  already present.
- Keep owner-specific lowering decisions in the owner that emits the records.
- Prefer small mechanical slices with focused proof.

## Out Of Scope

- Moving target-local register spelling or scratch selection into shared
  prealloc/BIR.
- Rewriting scalar cast, atomic, global, or memory semantics.
- Broad calls/dispatch/printer cleanup.

## Proof Expectations

- Focused build proof for each owner slice.
- Backend tests covering AArch64 globals, atomics, casts, and memory lowering
  before closure.
- Regression guard if helpers are shared across more than one owner.

## Reviewer Reject Signals

- A helper is moved to shared code without proving target neutrality.
- The route replaces real semantic lowering with testcase-shaped matches.
- Diagnostics or unsupported-path messages are weakened to make helper sharing
  easier.
- Multiple owner families are rewritten at once without an intermediate proof.

