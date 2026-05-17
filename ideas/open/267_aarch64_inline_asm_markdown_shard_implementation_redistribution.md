# AArch64 `inline_asm.md` Shard Implementation Redistribution

Status: Open
Created: 2026-05-17

## Intent

Reconcile `src/backend/mir/aarch64/codegen/inline_asm.md` back into real
compiled owners named `inline_asm.cpp` and `inline_asm.hpp`, then delete the
markdown shard.

The inline-asm owner should preserve current-route behavior and explicit
deferred boundaries. It should not turn the markdown reference into an
unbounded feature port.

## Why This Exists

Inline assembly has target-specific formatting, constraint, clobber, and
operand-substitution concerns. Leaving its shard as markdown makes it likely
that future inline-asm work will spread across `instruction.cpp`, `dispatch.cpp`,
`memory.cpp`, or the assembly printer instead of landing behind a clear
AArch64 owner.

## In Scope

- Create `src/backend/mir/aarch64/codegen/inline_asm.cpp` and
  `src/backend/mir/aarch64/codegen/inline_asm.hpp`.
- Move any existing AArch64 inline-asm handling or explicit deferred/fail-closed
  hooks into the inline-asm owner.
- Keep dispatch as routing into the inline-asm owner.
- Preserve the distinction between current supported inline-asm behavior and
  historical reference behavior.
- Delete `inline_asm.md` once reconciled.

## Out of Scope

- Full GCC constraint implementation unless already supported by current
  prepared facts.
- Porting the old reference substitution machinery without current tests and
  route contracts.
- Changing atomic lowering just because the reference shard shared helpers.
- Reworking the common MIR printer or assembler boundary.
- Redistributing other markdown shards.

## Completion Criteria

- `inline_asm.cpp` and `inline_asm.hpp` exist and own the current inline-asm
  boundary or explicit deferred handling.
- `inline_asm.md` is deleted.
- Broad owners do not contain new inline-asm bulk behavior.
- Focused backend proof shows existing behavior is preserved.

## Reviewer Reject Signals

Reject the route or slice if it:

- turns the old markdown into a broad unvalidated inline-asm feature port;
- weakens unsupported handling or test contracts to claim progress;
- leaves inline-asm behavior in broad owners after creating the files;
- leaves `inline_asm.md` in place after claiming reconciliation;
- mixes unrelated atomic, memory, or printer changes into the shard movement.
