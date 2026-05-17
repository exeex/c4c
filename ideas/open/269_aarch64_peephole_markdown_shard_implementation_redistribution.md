# AArch64 `peephole.md` Shard Implementation Redistribution

Status: Open
Created: 2026-05-17

## Intent

Reconcile `src/backend/mir/aarch64/codegen/peephole.md` back into real compiled
owners named `peephole.cpp` and `peephole.hpp`, then delete the markdown shard.

The current compiler should not silently revive a text-first peephole optimizer
unless there is an explicit current route for it. If no active peephole pass is
supported, the compiled owner should record a narrow no-op or deferred boundary
that keeps future work from spreading into unrelated files.

## Why This Exists

The old shard describes a post-text assembly optimizer. The modern AArch64
route is moving toward prepared BIR -> compiled MIR nodes -> shared printer or
future assembler. A peephole pass may still be useful later, but its ownership
must be explicit and must not obscure the compiled-module boundary.

## In Scope

- Create `src/backend/mir/aarch64/codegen/peephole.cpp` and
  `src/backend/mir/aarch64/codegen/peephole.hpp`.
- Decide whether the current route has an active peephole pass, no-op pass, or
  explicit deferred boundary.
- Keep any peephole behavior behind the peephole owner rather than in
  `asm_emitter`, `machine_printer`, or `backend.cpp`.
- Delete `peephole.md` once reconciled.
- Preserve emitted assembly unless an explicitly reviewed peephole behavior
  already exists and is being moved without semantic change.

## Out of Scope

- Reintroducing the old text optimizer wholesale.
- Performing new optimization work.
- Changing MIR instruction selection, register allocation, or printer spelling.
- Using peephole rewrites to make narrow tests pass.
- Redistributing other markdown shards.

## Completion Criteria

- `peephole.cpp` and `peephole.hpp` exist and represent the current peephole
  ownership boundary.
- `peephole.md` is deleted.
- The current compiled-module and shared-printer route remains clear.
- Focused backend proof confirms no unintended output changes.

## Reviewer Reject Signals

Reject the route or slice if it:

- adds text rewrites that change emitted assembly without an explicit
  optimization contract;
- uses peephole matching as a testcase-shaped shortcut;
- hides peephole behavior in the printer, asm emitter, or backend driver;
- leaves `peephole.md` in place after claiming reconciliation;
- weakens tests to accept changed assembly.
