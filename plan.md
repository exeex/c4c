# Prealloc Responsibility Map And Layout Planning Runbook

Status: Active
Source Idea: ideas/open/prealloc-responsibility-map-and-layout-plan.md

## Purpose

Build a responsibility map for `src/backend/prealloc` and turn it into a small
set of focused follow-up layout or consolidation ideas.

## Goal

Document what prealloc owns, identify high-value cleanup candidates, and create
actionable follow-up ideas without doing broad implementation movement in this
audit runbook.

## Core Rule

This is a discovery and planning runbook. Do not change implementation behavior,
move target-specific instruction emission into prealloc, weaken tests, or hide
semantic changes inside layout cleanup.

## Read First

- `ideas/open/prealloc-responsibility-map-and-layout-plan.md`
- `src/backend/prealloc/`
- `src/backend/prealloc/prepared_printer/`
- `src/backend/prealloc/regalloc/`
- `src/backend/prealloc/stack_layout/`
- nearby backend proof commands recorded in recent `todo.md` history when
  choosing validation for later follow-up ideas

## Current Targets

- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/out_of_ssa.cpp`
- `src/backend/prealloc/i128_runtime_helpers.cpp`
- `src/backend/prealloc/f128_runtime_helpers.cpp`
- `src/backend/prealloc/liveness.cpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/runtime_helpers.hpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/publication_plans.*`
- `src/backend/prealloc/prepared_lookups.*`
- `src/backend/prealloc/decoded_home_storage.*`
- `src/backend/prealloc/prepared_printer/`
- `src/backend/prealloc/regalloc/`
- `src/backend/prealloc/stack_layout/`

## Non-Goals

- Do not perform broad implementation cleanup in this audit.
- Do not split headers, move code, rename files, or rewrite CMake source lists
  unless a tiny mechanical probe is explicitly justified in `todo.md`.
- Do not move target-specific register spelling or final instruction emission
  into prealloc.
- Do not split data contracts so finely that targets need many tiny headers for
  one logical plan.
- Do not rewrite test expectations, mark supported tests unsupported, or claim
  progress through classification-only edits.
- Do not fold the umbrella entropy-reduction generator into this focused
  prealloc audit.

## Working Model

Treat `prealloc` as the target-parameterized preparation layer between semantic
BIR and target MIR/codegen. It may compute stable facts, plans, identities,
storage decisions, and target profile inputs that codegen consumes. It should
not own final target instruction selection, register spelling, or assembly
emission.

Use these responsibility categories when classifying files:

- pipeline coordination and phase ordering
- control-flow normalization, out-of-SSA, and move bundles
- liveness and register-allocation planning
- stack layout and frame planning
- call/return ABI movement plans
- value homes, storage encodings, and publication plans
- runtime helper carriers for i128/f128/atomics/intrinsics/inline asm
- dynamic stack and variadic entry plans
- target register profile facts
- prepared-module lookup/accessor helpers
- prepared printer/debug dump support

## Execution Rules

- Use `rg`, `find`, `wc -l`, or `scripts/count_src_lines.py` to build the
  inventory before drawing ownership conclusions.
- Keep audit output in `todo.md` unless it must become a durable follow-up idea.
- For symbol ownership questions that are not obvious from file boundaries, use
  `c4c-clang-tools` instead of guessing from raw text alone.
- Separate target-neutral, target-parameterized, and accidentally
  target-specific responsibilities.
- For generated follow-up ideas, include target files, slice type, durable
  owner, expected proof, and concrete reviewer reject signals.
- If any code-changing probe is delegated, require:
  `bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`
- For audit-only packets, `git diff --check` is sufficient proof.

## Step 1: Inventory Prealloc Files And Inputs

Goal: produce a complete inventory of prealloc files, sizes, and obvious broad
surfaces before proposing responsibility boundaries.

Primary target:
- `src/backend/prealloc/`

Actions:
- List every `.cpp` and `.hpp` under `src/backend/prealloc`, including nested
  directories.
- Record line counts and identify the largest implementation files and headers.
- Record broad headers and include-heavy surfaces that may mix unrelated data
  contracts.
- List helper names mentioning `fallback`, `compat`, `legacy`, `bridge`,
  `temporary`, `workaround`, or old route names.
- Record the inventory and first observations in `todo.md`.

Completion check:
- `todo.md` contains the file inventory, largest-file list, broad-header list,
  and suspicious-helper list.
- No implementation files are changed.

## Step 2: Build The Responsibility Map

Goal: classify each prealloc file by durable responsibility and identify mixed
ownership boundaries.

Primary target:
- the Step 1 inventory in `todo.md`

Actions:
- Assign every prealloc file to one primary responsibility category from the
  working model.
- Mark any file that deliberately spans categories and explain why that shape
  is durable or temporary.
- Identify files that are target-neutral, target-parameterized, or accidentally
  target-specific.
- Note prepared-printer files that cleanly mirror data families and files that
  should follow future layout moves.

Completion check:
- `todo.md` contains a file-to-responsibility map and mixed-boundary notes.
- Ambiguous ownership is called out explicitly instead of hidden by broad
  categories.

## Step 3: Propose The Stable Package Model

Goal: turn the responsibility map into a coherent internal package and family
model for prealloc.

Actions:
- Group files into proposed durable families for contracts, phases, helpers,
  runtime carriers, target profile facts, and prepared-printer support.
- Identify which headers should remain aggregate contracts and which are
  candidates for future contraction or contract split.
- Identify `.cpp` files that look like phase extraction, family merge, helper
  relocation, or naming repair candidates.
- Separate semantic ownership issues from layout cleanup candidates.

Completion check:
- `todo.md` contains the proposed package model and a prioritized cleanup
  candidate table grouped by slice type.
- Any semantic migration candidate is labeled as a separate initiative, not
  mixed into layout cleanup.

## Step 4: Create Focused Follow-Up Ideas

Goal: record the highest-value cleanup candidates as separate actionable source
ideas under `ideas/open/`.

Actions:
- Choose the safest and highest-reuse cleanup candidates from Step 3.
- Create multiple focused `ideas/open/*.md` follow-up ideas, each scoped to one
  cleanup slice.
- For each idea, include goal, target files, slice type, durable owner, expected
  behavior-preservation proof, out-of-scope work, and reviewer reject signals.
- Do not create a giant multi-purpose prealloc refactor idea.

Completion check:
- Follow-up idea files exist for the highest-value cleanup slices.
- Each generated idea is small enough for one focused agent run and has concrete
  reject signals.
- `todo.md` records the generated idea paths and recommended execution order.

## Step 5: Final Audit Review

Goal: decide whether the source idea is complete and whether the active plan can
be closed.

Actions:
- Review the final responsibility map, package model, and generated follow-up
  ideas against the source idea acceptance criteria.
- Confirm no broad code movement or semantic change was performed during the
  audit.
- Confirm that any remaining unresolved ownership question is either captured in
  `todo.md` or represented by a follow-up idea.
- Ask the plan owner to close only if the source idea itself is complete.

Completion check:
- `todo.md` contains the final map summary, generated idea list, proof summary,
  and closure recommendation.
- The source idea can be closed without leaving planning output only in chat.
