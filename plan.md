# AArch64 Codegen CPP Family Consolidation Runbook

Status: Active
Source Idea: ideas/open/aarch64-codegen-cpp-family-consolidation.md

## Purpose

Consolidate only the AArch64 codegen `.cpp` files whose current split no
longer represents a durable implementation boundary.

## Goal

Merge a small, reviewable set of connective or tiny `.cpp` files while keeping
large instruction-selection and lowering responsibilities in separate modules.

## Core Rule

This is a behavior-preserving file-boundary cleanup. Do not use consolidation
to move target-independent logic into AArch64, weaken tests, change lowering
semantics, or recreate oversized implementation files.

## Read First

- `ideas/open/aarch64-codegen-cpp-family-consolidation.md`
- `src/backend/mir/aarch64/codegen/README.md`
- Current `.cpp` layout under `src/backend/mir/aarch64/codegen/`
- Recent closed ideas for Prepared boundary recovery and header family
  consolidation only if historical context is needed

## Current Targets

- `src/backend/mir/aarch64/codegen/dispatch_publication_common.cpp`
- Small or connective files in the `calls*.cpp` family
- Any `.cpp` file that became trivial after Prepared boundary recovery

Use these as candidates, not as permission to merge every named file.

## Non-Goals

- Do not merge large files with distinct lowering responsibility.
- Do not merge across unrelated families just to reduce file count.
- Do not rename public concepts unless the merge makes the old name actively
  misleading.
- Do not touch header-family consolidation work that is already complete except
  for include fallout required by `.cpp` merges.
- Do not edit tests or expectations unless a build-system or include-path
  consequence genuinely requires it.

## Working Model

- Files such as `dispatch.cpp`, `calls_moves.cpp`, instruction selection,
  value materialization, memory, ALU, and other large feature modules are
  durable boundaries unless the audit proves otherwise.
- `dispatch_publication_common.cpp` is a likely publication helper merge
  candidate if its contents are local to `dispatch_publication.cpp`.
- Smaller call helper files may be merged into the nearest `calls` family
  implementation only when that produces a clearer ownership boundary.
- Files should remain separate when independent backend work is likely to edit
  them independently.

## Execution Rules

- Work in small family-level steps.
- Before each merge, record the rationale in `todo.md` and keep the source
  idea unchanged unless durable intent actually changes.
- Prefer pure move-and-include cleanup over behavior changes.
- After code-changing steps, run:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log
```

- Scan for stale build references or includes after deleting any `.cpp` file.
- Escalate to broader validation if a merge touches build definitions, shared
  AArch64 dispatch surfaces, or more than one family in the same slice.

## Step 1: Audit CPP Family Merge Candidates

Goal: decide which `.cpp` files are real merge candidates under the source idea
rules.

Actions:

- Inspect current `.cpp` sizes and include/build references in
  `src/backend/mir/aarch64/codegen`.
- Classify candidates by family: dispatch publication, calls helpers, and any
  newly trivial prepared-boundary fallout.
- Identify files that must stay separate because they are large, semantic, or
  likely to be independently modified.
- Record the chosen first implementation target and explicit non-targets in
  `todo.md`.

Completion check:

- `todo.md` names the first merge target, its destination file, files that stay
  separate, and the proof command for the implementation step.

## Step 2: Consolidate Dispatch Publication CPP Boundary

Goal: merge dispatch publication helper code only if the Step 1 audit confirms
it is connective and local to the publication implementation.

Primary target:

- `src/backend/mir/aarch64/codegen/dispatch_publication_common.cpp`
- nearest publication implementation, expected to be
  `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`

Actions:

- Move local helper definitions into the selected publication implementation.
- Delete the obsolete `.cpp` file only when no build target or include path
  still needs it.
- Update build manifests or source lists if the deleted `.cpp` was enumerated.
- Preserve function behavior and diagnostics.

Completion check:

- No stale references to the removed dispatch publication `.cpp` remain.
- Backend build and `^backend_` CTest subset pass with proof in
  `test_after.log`.

## Step 3: Consolidate Calls CPP Helpers

Goal: merge only call-family helper `.cpp` files whose boundaries are thin
glue after header consolidation.

Primary target:

- small or connective `src/backend/mir/aarch64/codegen/calls*.cpp` files
  selected by Step 1

Actions:

- Merge one calls-family boundary at a time into the nearest durable calls
  implementation.
- Keep large call lowering modules separate when their size or responsibility
  remains durable.
- Update build manifests or source lists for deleted `.cpp` files.
- Avoid broad renames unless needed to resolve local helper collisions caused
  by the merge.

Completion check:

- Deleted call helper `.cpp` files have no stale build or include references.
- Backend build and `^backend_` CTest subset pass with proof in
  `test_after.log`.

## Step 4: Boundary Review and Broader Validation

Goal: confirm the consolidation improved file boundaries without hiding
responsibilities or weakening behavior.

Actions:

- Re-scan `src/backend/mir/aarch64/codegen` for remaining tiny `.cpp` files and
  record whether each is intentionally retained or out of scope.
- Verify remaining file names still correspond to durable implementation
  concepts.
- Check that no target-independent logic moved into AArch64 during the merges.
- Run the supervisor-selected broader validation or regression guard if the
  accumulated slices justify it.

Completion check:

- `todo.md` records the final boundary rationale, validation proof, and any
  intentionally deferred candidates.
- The source idea acceptance criteria are satisfied or remaining work is
  explicitly identified for a separate idea.
