# AArch64 Cast-Ops Shard Redistribution Runbook

Status: Active
Source Idea: ideas/open/259_aarch64_cast_ops_markdown_shard_implementation_redistribution.md

## Purpose

Reconcile `src/backend/mir/aarch64/codegen/cast_ops.md` into compiled
`cast_ops.cpp` and `cast_ops.hpp` owners while preserving existing scalar cast
behavior.

Goal: make cast-specific AArch64 selected-node construction, lowering,
spelling, and diagnostics live in the cast-ops owner instead of broad codegen
files.

Core Rule: this is behavior-preserving source ownership redistribution, not a
cast semantics expansion or instruction hierarchy redesign.

## Activation Note

This plan resumes the paused cast-ops redistribution after closure of
`ideas/open/263_aarch64_codegen_public_compiled_module_interface.md`. The
linked source idea records that Step 2 had already introduced
`cast_ops.cpp`, `cast_ops.hpp`, and build registration with focused cast proof
passing. Resume execution at Step 3.

## Read First

- `ideas/open/259_aarch64_cast_ops_markdown_shard_implementation_redistribution.md`
- `src/backend/mir/aarch64/codegen/cast_ops.md`
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `src/backend/mir/aarch64/codegen/cast_ops.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- Nearby compiled shard pairs such as `alu.cpp`/`alu.hpp`,
  `comparison.cpp`/`comparison.hpp`, and `memory.cpp`/`memory.hpp`

## Current Scope

- Continue from the existing `src/backend/mir/aarch64/codegen/cast_ops.cpp`.
- Continue from the existing `src/backend/mir/aarch64/codegen/cast_ops.hpp`.
- Move valid cast-family behavior out of broad owners into the cast-ops files.
- Keep broad owners as routing or family-neutral infrastructure.
- Delete `src/backend/mir/aarch64/codegen/cast_ops.md` only after its durable
  valid content is reconciled into compiled owners.

## Non-Goals

- Do not redistribute other AArch64 markdown shards.
- Do not redistribute `mod.md` or `records.md`.
- Do not redesign `instruction.hpp` or the instruction type hierarchy only to
  split this shard.
- Do not restore a centralized record pile under a new name.
- Do not expand scalar cast semantics beyond behavior-preserving relocation.
- Do not weaken tests, mark supported cast paths unsupported, or rewrite
  expectations to claim progress.

## Working Model

Cast ownership belongs in `cast_ops.cpp`/`cast_ops.hpp` when the behavior is
cast-family construction, lowering, spelling, helper logic, or diagnostics.
`instruction.cpp` should retain family-neutral selected-node core.
`dispatch.cpp` should route to cast-ops code instead of containing cast bodies.
`machine_printer.cpp` should delegate cast-specific spelling when that spelling
can be owned by the cast-ops shard.

Each shard `.cpp` may include the existing `instruction.hpp`. Do not make
header reshaping a precondition for this redistribution.

## Execution Rules

- Preserve cast behavior first; move ownership in small compile-proven steps.
- Prefer existing AArch64 shard patterns over new abstractions.
- Keep source intent in the linked idea stable unless the durable goal changes.
- Update `todo.md` with packet progress and proof; do not churn this runbook
  for routine implementation notes.
- Treat testcase-shaped shortcuts, helper-only renames, and expectation-only
  movement as route failures.
- Before closure, prove the focused backend behavior and confirm
  `cast_ops.md` is removed.

## Ordered Steps

### Step 1: Inventory cast_ops shard and current owners

Goal: identify the valid cast-specific behavior described by `cast_ops.md` and
where the compiled implementation currently lives.

Primary targets:
- `src/backend/mir/aarch64/codegen/cast_ops.md`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

Actions:
- Inspect the markdown shard for durable cast behavior, dependencies, hidden
  assumptions, and rebuild risks.
- Locate cast-specific selected-node construction, lowering, spelling, helper,
  and diagnostic code in compiled owners.
- Compare nearby compiled shard pairs to choose the local split pattern for
  `cast_ops.cpp` and `cast_ops.hpp`.
- Record the first implementation packet and focused proof command in
  `todo.md`.

Completion check:
- The executor can name the cast behavior to move, the source files that own it
  today, and the first narrow compile/test proof before editing code.

### Step 2: Introduce compiled cast_ops owner shell

Goal: add the cast-ops source/header pair using existing shard conventions
without changing behavior.

Primary targets:
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `src/backend/mir/aarch64/codegen/cast_ops.hpp`
- build configuration for AArch64 codegen sources, if explicit registration is
  required

Actions:
- Add the new header and source with includes and namespace layout matching
  nearby shard owners.
- Export only the minimal interfaces needed by existing dispatch, instruction,
  or printer code.
- Keep function names and signatures aligned with local conventions.
- Run the narrow build proof after the owner shell compiles.

Completion check:
- The new files compile and introduce no behavior change beyond making a
  compiled cast-ops owner available.

### Step 3: Move cast construction and lowering behavior

Goal: relocate cast-family selected-node construction and lowering into the
cast-ops owner.

Primary targets:
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `src/backend/mir/aarch64/codegen/cast_ops.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`

Actions:
- Move cast-specific construction and lowering bodies into `cast_ops.cpp`.
- Leave `instruction.cpp` with only family-neutral selected-node core.
- Leave `dispatch.cpp` as routing to the cast-ops owner, not a home for
  cast-family bodies.
- Preserve F128 delegation/fail-closed assumptions described by the shard.
- Compile after each meaningful move if the ownership boundary is risky.

Completion check:
- Cast construction/lowering behavior is owned by `cast_ops.cpp` and existing
  scalar cast behavior still compiles under focused backend proof.

### Step 4: Move cast spelling and diagnostics

Goal: relocate cast-specific spelling and diagnostics that can be owned by the
cast-ops shard.

Primary targets:
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `src/backend/mir/aarch64/codegen/cast_ops.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

Actions:
- Move cast-specific printer spelling helpers or diagnostic formatting into
  the cast-ops owner when they are not family-neutral printer infrastructure.
- Keep `machine_printer.cpp` as generic printer routing and shared emission
  infrastructure.
- Preserve exact instruction spelling and diagnostics unless relocation
  requires a mechanically equivalent helper boundary.

Completion check:
- Cast-specific spelling no longer lives in `machine_printer.cpp` when it can
  be owned by cast-ops helpers, and focused proof still preserves behavior.

### Step 5: Retire the markdown shard

Goal: delete the obsolete `cast_ops.md` shard after compiled ownership covers
its valid content.

Primary target:
- `src/backend/mir/aarch64/codegen/cast_ops.md`

Actions:
- Re-read `cast_ops.md` against the compiled owner to confirm valid durable
  content has been reconciled.
- Delete `cast_ops.md`.
- Do not copy stale reference-only prose into code comments.
- Run focused backend proof after deletion.

Completion check:
- `cast_ops.md` is gone and the compiled owner files contain the valid
  behavior needed by the idea.

### Step 6: Final validation and closure readiness

Goal: prove the source idea is satisfied without unrelated feature expansion.

Actions:
- Run focused backend proof selected by the supervisor.
- Escalate to a broader AArch64/backend check if the implementation touched
  shared instruction, dispatch, or printer paths in a way that could affect
  more than cast operations.
- Confirm no supported cast path was downgraded, no expectation-only progress
  was claimed, and no named-case shortcut was added.
- Confirm the diff is limited to cast-ops redistribution and required build
  plumbing.

Completion check:
- The supervisor has enough proof to request plan-owner closure review for the
  linked source idea.
