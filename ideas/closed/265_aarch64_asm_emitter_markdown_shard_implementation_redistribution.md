# AArch64 `asm_emitter.md` Shard Implementation Redistribution

Status: Closed
Created: 2026-05-17
Last Updated: 2026-05-17

## Intent

Reconcile `src/backend/mir/aarch64/codegen/asm_emitter.md` with the active
compiled owners `asm_emitter.cpp` and `asm_emitter.hpp`, then delete the stale
markdown shard.

The current AArch64 text assembly consumer should remain a thin helper over the
compiled module plus shared MIR printer. This idea is about making the live
owner boundary honest, not resurrecting the old reference inline-assembly
emitter wholesale.

## Why This Exists

`asm_emitter.cpp` and `asm_emitter.hpp` already exist, but
`asm_emitter.md` still describes a much older inline-assembly oriented surface.
That makes review confusing because the file name now means the current `.s`
text output helper, while the markdown still talks about removed reference
inline-asm machinery.

## In Scope

- Audit the durable facts in `asm_emitter.md` against the current
  `asm_emitter.cpp`/`asm_emitter.hpp` responsibility.
- Keep `asm_emitter` focused on rendering a compiled AArch64 module through
  the shared MIR printer and target instruction spelling hooks.
- Move any valid current-route assembly-output helper code into
  `asm_emitter.cpp`/`asm_emitter.hpp` if it still lives in a broader owner.
- Delete `src/backend/mir/aarch64/codegen/asm_emitter.md` once reconciled.
- Preserve emitted `.s` behavior.

## Out of Scope

- Implementing full inline assembly lowering.
- Implementing a native assembler, object writer, or encoder.
- Reintroducing the old reference `InlineAsmEmitter` as a direct port.
- Changing machine instruction printing semantics.
- Redistributing other AArch64 codegen markdown shards.
- Restoring stale record-pile abstractions.

## Completion Criteria

- `asm_emitter.cpp` and `asm_emitter.hpp` are the live owners for the current
  AArch64 `.s` text rendering helper.
- Any current-route AArch64 assembly rendering code is no longer stranded in
  generic driver files.
- `asm_emitter.md` is deleted.
- Focused AArch64 backend/MIR proof and public assembly smoke preserve output
  behavior.

## Completion Notes

Closed after Step 4 behavior-preservation proof. The audit found the live
prepared-module-to-`.s` wrapper already belongs to `asm_emitter.cpp` and
`asm_emitter.hpp`, while instruction spelling remains correctly owned by
`machine_printer.*`. The stale
`src/backend/mir/aarch64/codegen/asm_emitter.md` shard was deleted and the
AArch64 classification index was updated to remove the stale reference.

Close-time regression guard used the matching backend scope from
`test_before.log` and `test_after.log` with behavior-preservation semantics:
both logs reported 139/139 passing backend tests, no new failures, and the
checker passed with `--allow-non-decreasing-passed`.

## Reviewer Reject Signals

Reject the route or slice if it:

- ports old inline-assembly reference behavior unrelated to the current
  compiled-module-to-`.s` route;
- leaves `asm_emitter.md` in place after claiming reconciliation;
- keeps AArch64 `.s` rendering owned by generic `backend.cpp`;
- changes emitted assembly or printer behavior while claiming ownership-only
  redistribution;
- weakens tests or smoke coverage instead of preserving behavior;
- restores the legacy centralized record pile under a new name.
