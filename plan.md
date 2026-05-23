# AArch64 Codegen Header Family Consolidation Plan

Status: Active
Source Idea: ideas/open/aarch64-codegen-header-family-consolidation.md

## Purpose

Reduce local header count in `src/backend/mir/aarch64/codegen` while preserving
the useful `.cpp` implementation split.

## Goal

Make prefix families include one navigable family header: `calls_*.cpp` should
depend on `calls.hpp`, and `dispatch_*.cpp` should depend on `dispatch.hpp`.

## Core Rule

Consolidate declarations and includes only. Do not merge `.cpp` files, move
backend responsibilities, weaken tests, or claim semantic capability progress.

## Read First

- `ideas/open/aarch64-codegen-header-family-consolidation.md`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- Current `calls_*.hpp` and `dispatch_*.hpp` files

## Current Targets

- Primary families:
  - `src/backend/mir/aarch64/codegen/calls*.hpp`
  - `src/backend/mir/aarch64/codegen/dispatch*.hpp`
- Primary include users:
  - `src/backend/mir/aarch64/codegen/calls*.cpp`
  - `src/backend/mir/aarch64/codegen/dispatch*.cpp`

## Non-Goals

- Do not merge `.cpp` files.
- Do not move logic between BIR, prealloc, MIR, and AArch64 codegen.
- Do not introduce broad umbrella headers outside these local families unless a
  build break proves a narrow local include is required.
- Do not rewrite diagnostics, lowering semantics, or test expectations.
- Do not start the `.cpp` family consolidation idea in this plan.

## Working Model

- Family headers are local interface indexes for one prefix family.
- Preserve navigability by grouping moved declarations by original subtopic.
- Prefer direct source includes of `calls.hpp` or `dispatch.hpp`; avoid leaving
  pass-through shard headers behind unless a survivor is explicitly justified.
- Treat any required survivor as a documented exception, not as the default.

## Execution Rules

- Keep each step behavior-preserving.
- Before deleting a shard header, move its declarations into the family header
  and update all local includes that referenced it.
- Preserve include order and formatting style where practical.
- Use `rg` to confirm no stale includes remain before each build proof.
- For code-changing steps, run:
  - `cmake --build --preset default`
  - a targeted backend subset chosen by the supervisor, normally
    `ctest --test-dir build -j --output-on-failure -R '^backend_'`

## Steps

### Step 1: Consolidate Calls Headers

Goal: fold `calls_*.hpp` declarations into `calls.hpp` and make calls-family
sources include the family header.

Primary target: `src/backend/mir/aarch64/codegen/calls.hpp`

Actions:

- Inspect each `calls_*.hpp` declaration group and its include users.
- Move declarations into `calls.hpp`, grouped by subtopic matching the old
  shard names.
- Update `calls_*.cpp` and any other local users from shard includes to
  `calls.hpp`.
- Delete calls shard headers that become empty.
- Document any calls shard header survivor in `todo.md` with the reason.

Completion check:

- `rg '#include "calls_' src/backend/mir/aarch64/codegen` finds no stale
  calls shard include unless `todo.md` records an explicit survivor.
- Fresh build and supervisor-selected backend proof pass.

### Step 2: Consolidate Dispatch Headers

Goal: fold `dispatch_*.hpp` declarations into `dispatch.hpp` and make
dispatch-family sources include the family header.

Primary target: `src/backend/mir/aarch64/codegen/dispatch.hpp`

Actions:

- Inspect each `dispatch_*.hpp` declaration group and its include users.
- Move declarations into `dispatch.hpp`, grouped by subtopic matching the old
  shard names.
- Update `dispatch_*.cpp` and any other local users from shard includes to
  `dispatch.hpp`.
- Delete dispatch shard headers that become empty.
- Document any dispatch shard header survivor in `todo.md` with the reason.

Completion check:

- `rg '#include "dispatch_' src/backend/mir/aarch64/codegen` finds no stale
  dispatch shard include unless `todo.md` records an explicit survivor.
- Fresh build and supervisor-selected backend proof pass.

### Step 3: Boundary Review and Broader Validation

Goal: verify the header-family consolidation is complete and has not drifted
into implementation merging or responsibility movement.

Actions:

- Re-scan `src/backend/mir/aarch64/codegen` for remaining `calls_*.hpp` and
  `dispatch_*.hpp` files.
- Confirm remaining headers, if any, have explicit survivor rationale in
  `todo.md`.
- Confirm no `.cpp` files were merged or renamed as part of this plan.
- Confirm no BIR/prealloc/MIR ownership moves were introduced.
- Run the supervisor-selected broader validation checkpoint.

Completion check:

- The source idea acceptance criteria are satisfied, including fresh build
  proof.
- Any survivor headers are intentionally justified.
- The follow-on CPP-family consolidation idea remains separate and unstarted.
