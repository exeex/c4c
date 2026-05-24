# AArch64 Codegen Reference Layout Consolidation Runbook

Status: Active
Source Idea: ideas/open/aarch64-codegen-reference-layout-consolidation.md

## Purpose

Reorganize `src/backend/mir/aarch64/codegen` file and header boundaries so the
visible C++ layout reads like the reference AArch64 codegen module inventory
instead of historical migration scaffolding.

## Goal

Map every current AArch64 codegen `.cpp` and `.hpp` file to a reference-model
family, then land small behavior-preserving consolidation packets that reduce
catch-all headers and rename, merge, or justify thin historical adapters.

## Core Rule

This is a source-layout and module-boundary cleanup. Do not change emitted
AArch64 behavior, weaken tests, move target-neutral responsibilities into
prealloc or BIR, or create testcase-shaped shortcuts.

## Read First

- `ideas/open/aarch64-codegen-reference-layout-consolidation.md`
- `ref/claudes-c-compiler/src/backend/arm/codegen/README.md`
- `src/backend/mir/aarch64/codegen/`
- `src/backend/mir/aarch64/CMakeLists.txt`
- top-level or backend CMake files that enumerate AArch64 codegen sources

## Current Targets

- Every `.cpp` and `.hpp` directly under
  `src/backend/mir/aarch64/codegen/`.
- Catch-all or historical scaffold headers, especially `dispatch.hpp`.
- `dispatch_*.cpp` shards whose names describe migration mechanics instead of
  durable module responsibility.
- Thin `calls_*.cpp` shards and related headers that may be merged, renamed, or
  justified under the calls family.
- Build source lists and include sites affected by mechanical moves or renames.

## Reference Families

Use these family names when mapping files:

- `codegen/module`
- `prologue`
- `calls`
- `memory`
- `alu`
- `comparison`
- `float_ops`
- `cast_ops`
- `f128`
- `i128_ops`
- `atomics`
- `intrinsics`
- `globals`
- `returns`
- `variadic`
- `inline_asm`
- `asm_emitter`
- `peephole`
- `machine_printer`
- `operands/instruction`
- `adapter/internal`

## Non-Goals

- Do not perform another prealloc, Prepared, BIR, or forward-migration audit.
- Do not move target-neutral responsibilities out of AArch64 in this idea; if
  one is discovered, record a separate `ideas/open/*.md` follow-up.
- Do not merge large cohesive target-emission files just to reduce file count.
- Do not create a single giant `dispatch.cpp` or new broad catch-all header.
- Do not rewrite expectations, mark supported tests unsupported, or claim
  progress through classification-only changes once implementation begins.

## Working Model

The reference README gives the target mental model for visible families. The
C++ implementation may keep multiple implementation shards per family where
that makes review easier, but each shard should expose a durable responsibility:
calls, memory, comparison, returns, inline asm, and so on. Adapter/internal is
allowed only for genuine boundary glue that is not yet part of a durable public
family interface.

## Execution Rules

- Use `rg` or `find` to keep the file inventory complete before each
  consolidation packet.
- For C++ code edits, use `c4c-clang-tools` where symbol ownership or call
  relationships are not obvious from local includes.
- Prefer header narrowing and thin-file cleanup before broader renames.
- Keep each implementation packet mechanical and reviewable.
- Update build files and include paths in the same packet as any file rename or
  move.
- For code-changing packets, run:
  `bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`
- Run `git diff --check` before handing back any code-changing packet.
- Treat a downgrade in test expectations, unsupported classification, or
  testcase-shaped file surgery as a route-quality failure.

## Step 1: Map AArch64 Codegen Files

Goal: produce a complete file-to-reference-family map and identify the first
low-risk consolidation packet.

Primary target:
- `src/backend/mir/aarch64/codegen/`

Actions:
- List every direct `.cpp` and `.hpp` file in the directory.
- Assign each file to exactly one reference family or to `adapter/internal`.
- For `dispatch.hpp` and each `dispatch_*.cpp`, record whether it should move
  toward a durable family header, become private adapter/internal glue, or stay
  temporarily justified.
- For `calls_*.cpp`, record which shards are durable implementation splits and
  which are thin enough to merge or rename.
- Record the prioritized consolidation list in `todo.md`, starting with a
  low-risk header or thin-file cleanup packet.

Completion check:
- `todo.md` contains the complete file-to-family map, the prioritized
  consolidation list, and the recommended Step 2 packet.
- No implementation files are changed for this audit-only step.

## Step 2: Reduce Catch-All Header Surface

Goal: narrow broad declarations that make unrelated AArch64 codegen families
depend on `dispatch.hpp` or similar catch-all interfaces.

Primary targets:
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- any family header identified by Step 1 as the correct destination
- include sites affected by declaration movement

Actions:
- Move declarations that clearly belong to one durable family into that
  family's header.
- Make declarations private to a `.cpp` where they have only one implementation
  user.
- Keep adapter/internal declarations behind a narrow private boundary instead
  of broadening a family header.
- Update includes mechanically and avoid semantic changes.

Completion check:
- The chosen header surface is visibly narrower or explicitly justified in
  `todo.md`.
- The delegated backend proof command passes.
- `git diff --check` passes.

## Step 3: Rename Or Merge Thin Historical Shards

Goal: make thin files named for old migration mechanics read as durable module
responsibilities.

Primary targets:
- `src/backend/mir/aarch64/codegen/dispatch_*.cpp`
- thin `src/backend/mir/aarch64/codegen/calls_*.cpp` files selected by Step 1
- build source lists that enumerate renamed or merged files

Actions:
- Rename a thin shard into its reference-model family when that is a mechanical
  improvement.
- Merge a thin shard into an existing cohesive family file only when the result
  stays reviewable.
- Leave large cohesive files separate and record the justification in
  `todo.md`.
- Update CMake source lists and includes in the same packet.

Completion check:
- At least one historical scaffold filename is removed, renamed, merged, or
  documented as intentionally durable.
- The delegated backend proof command passes.
- `git diff --check` passes.

## Step 4: Consolidate Remaining Family Boundaries

Goal: repeat focused consolidation for the next highest-priority families
without creating new broad interfaces.

Actions:
- Use the Step 1 prioritized list to select the next narrow family-boundary
  cleanup.
- Keep each packet scoped to one family or one adapter boundary.
- Record any target-neutral migration candidate as a separate open idea instead
  of expanding this layout cleanup.
- Refresh the file-to-family map in `todo.md` after each meaningful layout
  packet.

Completion check:
- The codegen directory reads closer to the reference module inventory across
  more than one family.
- Remaining adapter/internal files are justified or queued for a later packet.
- The delegated backend proof command passes for each code-changing packet.
- `git diff --check` passes.

## Step 5: Final Layout Review

Goal: decide whether the source idea is complete or whether remaining layout
work needs a follow-up runbook.

Actions:
- Count final `.cpp` and `.hpp` files directly under
  `src/backend/mir/aarch64/codegen`.
- Compare the final file-to-family map against the reference README inventory.
- Record remaining intentionally separate files and why they stay separate.
- Confirm no target-neutral migration candidate was hidden inside this layout
  cleanup; create a separate `ideas/open/*.md` follow-up if needed.
- Ask the plan owner to close only if the source idea itself is complete.

Completion check:
- `todo.md` contains the final counts, remaining-file notes, proof summary, and
  closure recommendation.
- Any distinct remaining initiative is recorded under `ideas/open/` before
  closure.
