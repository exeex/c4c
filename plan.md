# AArch64 Emit Shard Implementation Redistribution Runbook

Status: Active
Source Idea: ideas/open/258_aarch64_emit_markdown_shard_implementation_redistribution.md

## Purpose

Reconcile `src/backend/mir/aarch64/codegen/emit.md` into real compiled owners
at `src/backend/mir/aarch64/codegen/emit.cpp` and
`src/backend/mir/aarch64/codegen/emit.hpp` without changing backend behavior.

## Goal

Make `emit.cpp`/`emit.hpp` the ordinary home for thin top-level module
emit/build orchestration while keeping instruction, dispatch, and printer
responsibilities in their narrower owners.

## Core Rule

This is source ownership redistribution only. Preserve existing emit/build
behavior and do not expand AArch64 backend capability, weaken tests, or recreate
the old centralized emitter under a new name.

## Read First

- `ideas/open/258_aarch64_emit_markdown_shard_implementation_redistribution.md`
- `src/backend/mir/aarch64/codegen/emit.md`
- `src/backend/mir/aarch64/codegen/emit.cpp`
- `src/backend/mir/aarch64/codegen/emit.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

## Current Targets

- `src/backend/mir/aarch64/codegen/emit.cpp`
- `src/backend/mir/aarch64/codegen/emit.hpp`
- `src/backend/mir/aarch64/codegen/emit.md`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`

## Non-Goals

- Do not redistribute any other AArch64 codegen markdown shard.
- Do not redistribute `mod.md`.
- Do not redesign `instruction.hpp` or the instruction type hierarchy solely
  for this shard.
- Do not move selected-node families, instruction spelling bodies, or backend
  semantics into `emit.cpp`.
- Do not downgrade expectations, mark supported paths unsupported, or use
  expectation rewrites as proof of progress.

## Working Model

`emit.cpp` should stay thin and own orchestration around prepared module
building, target-profile handoff, compatibility projection, and calls into
existing shard helpers. `dispatch.cpp` should route block and instruction
families into shard owners. `instruction.cpp` should retain family-neutral
instruction record helpers. `machine_printer.cpp` should own spelling/printing
surfaces rather than pushing printer bodies into `emit.cpp`.

## Execution Rules

- Move behavior only when the destination is the natural compiled owner named
  by the source idea.
- Prefer small behavior-preserving commits with focused build proof.
- Keep `emit.md` until its durable valid content has either been reconciled or
  classified as stale/out-of-scope for close.
- Record reconciliation notes in `todo.md` while executing; source-idea edits
  are reserved for lifecycle close or true intent changes.
- Treat testcase-shaped shortcuts, expectation rewrites, and broad cleanup as
  route drift.

## Step 1: Inventory Emit Shard Content

Goal: classify the durable content in `emit.md` against current compiled
owners before moving code.

Primary target: `src/backend/mir/aarch64/codegen/emit.md`

Actions:

- Inspect `emit.md` and identify content that is still valid for the current
  prepared-module record path.
- Compare that content with existing behavior in `emit.cpp`/`emit.hpp`,
  `dispatch.cpp`, `instruction.cpp`, and `machine_printer.cpp`.
- Separate valid emit/build orchestration from stale legacy direct-text emitter
  details and out-of-scope backend rebuild notes.
- Write the working classification in `todo.md` so later packets can reconcile
  it without editing the source idea.

Completion check:

- `todo.md` records which `emit.md` sections require compiled reconciliation,
  which are stale, and which are out of scope for this idea.
- No implementation behavior has changed.

## Step 2: Move Emit/Build Orchestration Into Emit Owners

Goal: make `emit.cpp`/`emit.hpp` own the valid thin top-level emit/build
orchestration described by the shard.

Primary targets:

- `src/backend/mir/aarch64/codegen/emit.cpp`
- `src/backend/mir/aarch64/codegen/emit.hpp`

Actions:

- Move only top-level module build orchestration that belongs to the emit shard
  into `emit.cpp`/`emit.hpp`.
- Keep `emit.cpp` as a coordinator over prepared-module target seams and
  existing shard helpers.
- Preserve target-profile resolution, validation, diagnostics, compatibility
  projection, and module record construction behavior.
- Avoid restoring old direct BIR/LIR text-emitter APIs unless the current
  compiled surface already requires them.

Completion check:

- The valid emit/build orchestration from Step 1 has a compiled owner in
  `emit.cpp`/`emit.hpp`.
- Focused build proof passes.

## Step 3: Tighten Adjacent Owner Boundaries

Goal: ensure broad owners do not retain emit-shard orchestration or absorb
responsibilities that belong elsewhere.

Primary targets:

- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`

Actions:

- Keep `instruction.cpp` focused on family-neutral instruction core and record
  helper behavior.
- Keep `dispatch.cpp` as routing into shard owners, not a container for family
  bodies.
- Keep printer spelling bodies in printer-owned helpers when they are not thin
  emit/build orchestration.
- If an adjacent owner already has the right responsibility, leave it in place
  and record that classification in `todo.md`.

Completion check:

- Adjacent owner boundaries match the source idea.
- No broad instruction, dispatch, or printer responsibility was moved into
  `emit.cpp` as a catch-all.
- Focused build proof passes.

## Step 4: Delete Reconciled Emit Markdown Shard

Goal: remove `emit.md` only after its durable content is accounted for.

Primary target: `src/backend/mir/aarch64/codegen/emit.md`

Actions:

- Confirm every valid durable `emit.md` point has a compiled owner or is
  captured by a close-time stale/out-of-scope classification.
- Delete `emit.md`.
- Keep any close ledger concise and tied to the classification from Step 1.

Completion check:

- `emit.md` is deleted.
- The close handoff can explain where durable content went and why any
  remaining markdown content was stale or out of scope.

## Step 5: Prove Preservation And Prepare Close

Goal: show the redistribution preserved behavior and is ready for lifecycle
close review.

Primary targets:

- `test_before.log`
- `test_after.log`
- focused backend proof chosen by the supervisor

Actions:

- Run the focused backend proof selected for the changed surface.
- Escalate to broader validation if orchestration movement crosses multiple
  backend buckets.
- Check the final diff for expectation downgrades, testcase-shaped shortcuts,
  and unrelated feature expansion.
- Hand the completed state to the supervisor for regression-guard and close
  decisions.

Completion check:

- Focused backend proof is green.
- Regression logs are in canonical form when required by the supervisor.
- The source idea completion criteria are satisfied without unrelated changes.
