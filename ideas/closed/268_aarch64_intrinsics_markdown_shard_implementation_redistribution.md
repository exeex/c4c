# AArch64 `intrinsics.md` Shard Implementation Redistribution

Status: Closed
Created: 2026-05-17
Closed: 2026-05-17

## Intent

Reconcile `src/backend/mir/aarch64/codegen/intrinsics.md` back into real
compiled owners named `intrinsics.cpp` and `intrinsics.hpp`, then delete the
markdown shard.

The intrinsics owner should collect current accepted target-intrinsic lowering
or explicit deferred handling without expanding semantics opportunistically.

## Why This Exists

The markdown shard documents a large historical intrinsic surface, but current
AArch64 codegen should not keep intrinsic-family decisions in broad files. A
dedicated compiled owner makes future intrinsic support easier to review and
prevents `instruction.cpp`, `dispatch.cpp`, or `float_ops.cpp` from becoming
the accidental home for unrelated platform builtins.

## In Scope

- Create `src/backend/mir/aarch64/codegen/intrinsics.cpp` and
  `src/backend/mir/aarch64/codegen/intrinsics.hpp`.
- Move any existing intrinsic dispatch/lowering/deferred hooks into the
  intrinsics owner.
- Keep platform intrinsic support explicit and grouped by intrinsic family.
- Keep unsupported intrinsics fail-closed or visibly deferred.
- Delete `intrinsics.md` once reconciled.

## Out of Scope

- Porting all reference NEON, CRC, cache, hint, frame-address, or soft-float
  helper behavior in one slice.
- Inventing missing prepared intrinsic facts.
- Changing scalar float, F128, memory, or call lowering outside an intrinsic
  ownership need.
- Rewriting tests or expectations to overstate intrinsic support.
- Redistributing other markdown shards.

## Completion Criteria

- `intrinsics.cpp` and `intrinsics.hpp` exist and own the current intrinsic
  boundary.
- `intrinsics.md` is deleted.
- Intrinsic behavior is not centralized in family-neutral owners.
- Focused backend proof covers any affected intrinsic or dispatch paths.

## Closure Notes

Closed after the active runbook completed the compiled AArch64 intrinsics
owner split, deleted the stale markdown shard, and recorded focused backend
proof in `test_after.log`. The close-time regression guard used matching
backend before/after logs and passed with non-decreasing results:
139/139 before and 139/139 after.

## Reviewer Reject Signals

Reject the route or slice if it:

- adds named-intrinsic shortcuts without a general target-intrinsic boundary;
- claims support mainly through expectation rewrites or weaker unsupported
  contracts;
- ports broad reference intrinsic behavior without current route facts;
- leaves `intrinsics.md` in place after claiming reconciliation;
- drags unrelated float, memory, call, or printer rewrites into this shard.
