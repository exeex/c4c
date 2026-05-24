# Prealloc Regalloc Coordinator Contraction Runbook

Status: Active
Source Idea: ideas/open/prealloc-regalloc-coordinator-contraction.md

## Purpose

Contract `regalloc.cpp` coordinator internals along existing
`src/backend/prealloc/regalloc/` helper-family boundaries while preserving
allocation behavior and dump meaning.

## Goal

Make allocation planning responsibilities easier to locate without changing
register allocation, spill/reload, interval, liveness, or ABI facts.

## Core Rule

Every code change must be behavior-preserving. Prefer one narrow helper
extraction, relocation, or naming clarification per packet, and keep
`regalloc.hpp` as the aggregate public allocation-plan contract unless a stable
smaller contract is proven by existing consumers.

## Read First

- `ideas/open/prealloc-regalloc-coordinator-contraction.md`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/regalloc.hpp`
- `src/backend/prealloc/regalloc/`
- `src/backend/prealloc/liveness.hpp`
- `src/backend/prealloc/prepared_printer/regalloc.cpp`

## Current Targets

- Primary coordinator: `src/backend/prealloc/regalloc.cpp`
- Helper family: `src/backend/prealloc/regalloc/`
- Public aggregate contract: `src/backend/prealloc/regalloc.hpp`
- Related facts: `src/backend/prealloc/liveness.*`,
  `src/backend/prealloc/value_locations.hpp`,
  `src/backend/prealloc/regalloc_placement_identity.*`
- Printer mirrors:
  `src/backend/prealloc/prepared_printer/regalloc.cpp`,
  `src/backend/prealloc/prepared_printer/value_locations.cpp`

## Non-Goals

- Do not change allocation decisions, spill/reload behavior, interval
  construction, value homes, call-return ABI facts, or liveness semantics.
- Do not replace the allocator architecture.
- Do not fragment `regalloc.hpp` before implementation helper boundaries are
  stable and consumers prove a smaller contract.
- Do not move concrete target register profile ownership out of
  `target_register_profile.*`.
- Do not add target-shaped shortcuts or named-case allocation rules.

## Working Model

- Treat liveness and allocation planning as the durable owner.
- Treat `regalloc.cpp` as a coordinator that should expose less unrelated
  detail over time.
- Use the existing `regalloc/` helper-family structure as the preferred home
  for extracted implementation detail.
- Keep prepared-printer labels aligned with any renamed allocation-plan
  concepts.

## Execution Rules

- Start with an inventory packet before changing code.
- Use `todo.md` for packet progress and proof notes.
- For code-changing packets, run:

```sh
bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'
```

- Always run `git diff --check`.
- If a candidate extraction requires broad mutable coordinator access, record
  the reason in `todo.md` and choose a smaller boundary.
- If a rename changes printed dump meaning, update the prepared-printer mirror
  in the same packet or defer the rename.

## Step 1 - Inventory Coordinator And Helper Families

Goal: map existing coordinator internals to durable helper-family boundaries.

Primary target: `src/backend/prealloc/regalloc.cpp`

Actions:
- Inspect `regalloc.cpp` top-level helpers and major coordinator phases.
- Compare them with existing files under `src/backend/prealloc/regalloc/`.
- Identify which helpers already belong to placement, spill/reload, interval,
  ABI fact, liveness, or printer responsibilities.
- Record one or two safest Step 2 candidates in `todo.md`.

Completion check:
- `todo.md` explains current ownership boundaries, rejected broad moves, and the
  first safe code-changing packet.
- `git diff --check` passes.

## Step 2 - Contract One Clear Coordinator Boundary

Goal: perform one behavior-preserving helper extraction, relocation, or naming
clarification that reduces unrelated detail in `regalloc.cpp`.

Primary target: chosen by Step 1.

Actions:
- Move or rename only the boundary selected in `todo.md`.
- Keep public aggregate contracts stable unless the change is provably
  implementation-local.
- Preserve all allocation-plan facts and dump meanings.

Completion check:
- The diff is small and aligned to an existing helper family.
- Backend proof passes.
- `git diff --check` passes.
- `todo.md` records why the boundary is behavior-preserving.

## Step 3 - Contract A Second Stable Boundary

Goal: repeat the Step 2 pattern for the next safest helper-family boundary, if
one remains clear after the first extraction.

Primary target: chosen by Step 2 results.

Actions:
- Prefer implementation-local movement or naming repair over public contract
  changes.
- Do not combine unrelated phase, interval, spill/reload, and ABI edits.
- Update printer mirrors only if public allocation-plan names or dump meanings
  changed.

Completion check:
- The coordinator exposes less unrelated detail or has clearer helper naming.
- Backend proof passes.
- `git diff --check` passes.
- `todo.md` records any deferred larger contract work.

## Step 4 - Audit Printer And Public Contract Alignment

Goal: verify that prepared-printer output and public allocation-plan contracts
still match the final helper boundaries.

Primary targets:
- `src/backend/prealloc/regalloc.hpp`
- `src/backend/prealloc/prepared_printer/regalloc.cpp`
- `src/backend/prealloc/prepared_printer/value_locations.cpp`

Actions:
- Audit dump labels and printed allocation-plan field meanings.
- Make only behavior-preserving mirror edits if a previous rename requires one.
- Do not split `regalloc.hpp` unless earlier steps proved a stable smaller
  contract and the change remains narrow.

Completion check:
- `todo.md` records whether printer changes were needed.
- Backend proof passes for code or printer changes; otherwise `git diff --check`
  passes for no-code audit.

## Step 5 - Closure Audit

Goal: decide whether the source idea is complete or whether a separate
follow-up idea is needed.

Actions:
- Compare completed packets against the source idea acceptance criteria.
- Confirm there were no behavior changes or overfit shortcuts.
- If remaining work is a distinct initiative, record the need for a new
  `ideas/open/` file rather than expanding this plan.

Completion check:
- `todo.md` recommends closure, deactivation, or follow-up creation.
- `git diff --check` passes.
