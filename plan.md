# AArch64 `asm_emitter.md` Shard Redistribution Runbook

Status: Active
Source Idea: ideas/open/265_aarch64_asm_emitter_markdown_shard_implementation_redistribution.md

## Purpose

Make the live AArch64 assembly text-output owner match the current compiled
implementation, then remove the stale markdown shard.

## Goal

Reconcile `src/backend/mir/aarch64/codegen/asm_emitter.md` into
`asm_emitter.cpp` and `asm_emitter.hpp` without changing emitted `.s` behavior.

## Core Rule

This is an ownership and stale-shard removal route. Preserve current assembly
printing semantics and do not resurrect legacy inline-assembly reference code.

## Read First

- `ideas/open/265_aarch64_asm_emitter_markdown_shard_implementation_redistribution.md`
- `src/backend/mir/aarch64/codegen/asm_emitter.md`
- `src/backend/mir/aarch64/codegen/asm_emitter.cpp`
- `src/backend/mir/aarch64/codegen/asm_emitter.hpp`
- nearby AArch64 MIR/backend tests or public assembly smoke tests that exercise
  `.s` output

## Current Targets

- Current AArch64 compiled-module-to-`.s` rendering helper ownership.
- Any AArch64 assembly rendering helper code still stranded outside
  `asm_emitter.cpp` / `asm_emitter.hpp`.
- Deletion of `src/backend/mir/aarch64/codegen/asm_emitter.md` after
  reconciliation.

## Non-Goals

- Do not implement full inline assembly lowering.
- Do not add a native assembler, object writer, or encoder.
- Do not port the old reference `InlineAsmEmitter` as a direct implementation.
- Do not change machine instruction printing semantics.
- Do not redistribute other AArch64 codegen markdown shards.
- Do not restore the old centralized record-pile model under a new name.

## Working Model

`asm_emitter.cpp` and `asm_emitter.hpp` should remain thin live owners for
rendering compiled AArch64 modules through the shared MIR printer and target
instruction spelling hooks. The markdown shard is only a reconciliation source:
extract durable current-route facts, ignore obsolete inline-assembly machinery,
and delete the shard once the live owner boundary is honest.

## Execution Rules

- Prefer small behavior-preserving moves over broad rewrites.
- Treat any emitted assembly diff as suspicious unless the source idea is
  explicitly revised.
- If markdown content describes removed inline-assembly machinery, document the
  rejection in `todo.md` rather than porting it.
- Keep validation tied to assembly output behavior, not just successful builds.
- Do not weaken, delete, or reclassify tests to claim progress.

## Ordered Steps

### Step 1: Audit The Shard Against The Live Owner

Goal: Identify which facts in `asm_emitter.md` still describe the current
compiled-module-to-`.s` route.

Primary target: `src/backend/mir/aarch64/codegen/asm_emitter.md`

Actions:

- Inspect the markdown shard and classify each durable section as current,
  obsolete inline-assembly reference material, or already represented in the
  compiled owner.
- Inspect `asm_emitter.cpp` and `asm_emitter.hpp` to confirm their current
  public surface and ownership boundary.
- Search for AArch64 assembly rendering helper logic in broader owners such as
  backend drivers, but avoid expanding into unrelated shard redistribution.
- Record the audit result in `todo.md` before making code moves.

Completion check:

- `todo.md` names the current facts to keep, the obsolete facts to reject, and
  any concrete helper ownership issue to fix.

### Step 2: Move Or Tighten Current-Route Ownership

Goal: Ensure current AArch64 `.s` rendering helpers live under
`asm_emitter.cpp` / `asm_emitter.hpp`.

Primary target: `src/backend/mir/aarch64/codegen/asm_emitter.cpp`

Actions:

- Move only valid current-route AArch64 assembly-output helper code into the
  asm emitter owner if it is still stranded in a broader implementation file.
- Keep the API thin and aligned with the existing compiled module rendering
  route.
- Preserve use of shared MIR printer and target instruction spelling hooks.
- Avoid adding testcase-shaped shortcuts or legacy inline-assembly abstractions.

Completion check:

- The live compiled owner boundary is honest, and a focused build or compile
  proof succeeds.

### Step 3: Delete The Stale Markdown Shard

Goal: Remove `asm_emitter.md` after all durable current facts have either been
absorbed or explicitly rejected as obsolete.

Primary target: `src/backend/mir/aarch64/codegen/asm_emitter.md`

Actions:

- Delete the markdown shard.
- Confirm no source list, docs index, or build/test reference still expects the
  deleted markdown file.
- Keep neighboring markdown shards untouched.

Completion check:

- `asm_emitter.md` is gone, and no repository references require it.

### Step 4: Prove Behavior Preservation

Goal: Show that ownership redistribution did not change emitted assembly.

Primary target: focused AArch64 backend/MIR and public assembly smoke coverage.

Actions:

- Run the narrow proof command chosen by the supervisor for the implementation
  packet.
- Include a public assembly smoke or equivalent output-preservation check when
  available.
- Escalate to broader backend validation if helper movement crosses driver or
  shared printer boundaries.

Completion check:

- Fresh proof is recorded in `test_after.log` by the executor, and `todo.md`
  summarizes the preserved behavior evidence.

## Completion Criteria

- `asm_emitter.cpp` and `asm_emitter.hpp` are the live owners for current
  AArch64 `.s` text rendering.
- Current-route AArch64 assembly rendering code is not stranded in generic
  driver files.
- `src/backend/mir/aarch64/codegen/asm_emitter.md` is deleted.
- Focused AArch64 backend/MIR proof and public assembly smoke preserve output
  behavior.
