# AArch64 `cast_ops.md` Shard Implementation Redistribution

Status: Open
Created: 2026-05-17

## Intent

Reconcile `src/backend/mir/aarch64/codegen/cast_ops.md` back into real
compiled owners named `cast_ops.cpp` and `cast_ops.hpp`. The cast-ops shard
should have an ordinary source/header home that owns valid scalar cast
selected-node construction, lowering, spelling, and diagnostics instead of
leaving cast-specific bodies stranded in broader codegen files.

Each shard `.cpp` may continue including the existing `instruction.hpp`. The
objective is source ownership redistribution, not a large instruction header
or instruction type redesign.

## Why This Exists

The AArch64 codegen markdown shards describe family-specific implementation
boundaries, but the cast-ops shard still needs to be reconciled into compiled
owners. Scalar cast construction, lowering, spelling, and diagnostics may be
centralized in files such as `instruction.cpp`, `dispatch.cpp`, or
`machine_printer.cpp`, making the shard layout misleading and keeping
family-specific code in family-neutral owners.

## In Scope

- Move cast-specific selected-node construction, lowering, helper, spelling,
  and diagnostic behavior from broad owners into `cast_ops.cpp`/`cast_ops.hpp`
  where the behavior belongs.
- Keep `instruction.cpp` focused on family-neutral instruction core rather
  than cast-family bulk implementation.
- Keep `dispatch.cpp` as routing into the cast-ops owner rather than a place
  that contains the cast-family body.
- Keep `machine_printer.cpp` from retaining cast-specific spelling bodies when
  those can be owned through cast-ops shard helpers.
- Delete `src/backend/mir/aarch64/codegen/cast_ops.md` once its durable valid
  content has been reconciled into the compiled owner files.
- Preserve existing scalar cast behavior while moving ownership.

## Out of Scope

- Redistributing any other AArch64 codegen `.md` shard.
- Redistributing `mod.md`; that file is an index byproduct, not a normal
  implementation redistribution shard for this batch.
- Redistributing `records.md`; that file is a stale record-pile special case,
  not a normal implementation redistribution shard for this batch.
- Broad AArch64 codegen cleanup unrelated to the `cast_ops.md` shard.
- Expanding cast semantics beyond what is necessary for behavior-preserving
  relocation.
- Restoring or recreating a legacy centralized record pile under a new name.
- Rewriting tests or expectations to make the movement appear complete.
- Redesigning `instruction.hpp` or the instruction type hierarchy solely to
  split this shard.

## Completion Criteria

- `src/backend/mir/aarch64/codegen/cast_ops.cpp` and
  `src/backend/mir/aarch64/codegen/cast_ops.hpp` exist and own the valid
  cast-ops shard behavior that was previously described by `cast_ops.md`.
- `src/backend/mir/aarch64/codegen/cast_ops.md` is deleted before this idea
  closes.
- `instruction.cpp` retains only family-neutral instruction core, not
  cast-family bulk implementation.
- `dispatch.cpp` routes to the cast-ops owner rather than containing
  cast-family bodies.
- `machine_printer.cpp` does not retain cast-specific spelling bodies when
  that spelling can be owned through cast-ops shard helpers.
- Focused backend proof shows behavior is preserved.
- The completed diff does not include unrelated feature expansion.

## Paused State

Paused on 2026-05-17 to prioritize
`ideas/open/263_aarch64_codegen_public_compiled_module_interface.md`.

Resume from the active runbook's former Step 3: move scalar cast record
construction into the cast-ops owner without changing behavior. Step 2 had
introduced `src/backend/mir/aarch64/codegen/cast_ops.cpp`,
`src/backend/mir/aarch64/codegen/cast_ops.hpp`, and build registration, with
focused cast proof passing:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_.*cast'
```

## Reviewer Reject Signals

Reject the route or slice if it:

- downgrades test expectations, marks supported cast paths unsupported, or
  weakens contracts without explicit user approval;
- adds named-case shortcuts, testcase-shaped matching, or one-off scalar cast
  spellings instead of preserving the real construction/lowering/printing
  behavior;
- claims progress mainly through helper renames, classification changes, or
  expectation rewrites while cast-ops implementation remains centralized
  elsewhere;
- expands scalar cast semantics or diagnostics beyond behavior-preserving
  relocation;
- moves broad instruction, dispatch, or printer responsibilities into
  `cast_ops.cpp` that are not cast-family behavior;
- leaves `cast_ops.md` in place after claiming the shard has a compiled owner;
- restores the legacy centralized record pile behind a new abstraction name;
- changes cast semantics except where strictly required for
  behavior-preserving relocation.
