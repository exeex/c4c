# AArch64 `atomics.md` Shard Implementation Redistribution

Status: Open
Created: 2026-05-17

## Intent

Reconcile `src/backend/mir/aarch64/codegen/atomics.md` back into real compiled
owners named `atomics.cpp` and `atomics.hpp`, then delete the markdown shard.

The atomics owner should contain only valid current AArch64 atomic lowering or
explicit fail-closed/deferred handling. It should not be a direct unreviewed
port of the old reference backend.

## Why This Exists

The markdown shard documents an AArch64 atomic lowering boundary, but there is
no corresponding active C++ owner yet. If atomics behavior is added later while
the shard remains markdown-only, it will likely land in broad files such as
`instruction.cpp`, `dispatch.cpp`, or `memory.cpp`, repeating the same
centralization problem this cleanup series is trying to remove.

## In Scope

- Create `src/backend/mir/aarch64/codegen/atomics.cpp` and
  `src/backend/mir/aarch64/codegen/atomics.hpp`.
- Move or add current accepted atomic construction/lowering/diagnostic hooks
  into the atomics owner when prepared facts support them.
- Keep unsupported atomic forms explicitly deferred or fail-closed rather than
  silently lowered through generic placeholders.
- Route dispatch through the atomics owner instead of embedding atomic-family
  bodies in broad owners.
- Delete `atomics.md` once durable valid content has been reconciled.

## Out of Scope

- Inventing missing prepared atomic facts.
- Implementing all AArch64 exclusive-loop lowering from the reference shard in
  one broad jump.
- Changing memory, register allocation, frame, or printer behavior unrelated
  to atomic operations.
- Rewriting tests or expectations to make unsupported atomics look supported.
- Redistributing other markdown shards.

## Completion Criteria

- `atomics.cpp` and `atomics.hpp` exist and own the current atomic-family
  boundary.
- `atomics.md` is deleted.
- Broad owners do not contain new atomic-family bulk implementation.
- Supported behavior is preserved, and unsupported behavior remains explicit.
- Focused backend proof covers the affected AArch64 codegen route.

## Reviewer Reject Signals

Reject the route or slice if it:

- adds testcase-shaped atomic shortcuts or named-case-only lowering;
- claims full atomic support by expectation rewrites or unsupported-test
  downgrades;
- implements reference-exclusive loops without prepared facts or current route
  validation;
- leaves atomic behavior centralized in broad owners;
- leaves `atomics.md` in place after claiming the shard has a compiled owner;
- changes unrelated memory or ABI behavior.
