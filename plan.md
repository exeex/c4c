# BIR Route4 Publication Body Extraction Runbook

Status: Active
Source Idea: ideas/open/524_bir_route4_publication_body_extraction.md

## Purpose

Extract route4 current-block and block-entry publication implementation bodies
from `src/backend/bir/bir.cpp` into a focused owner without changing public
route4 declarations or publication semantics.

## Goal

Make route4 publication availability a clear implementation unit before route5
and route6 cleanup proceed.

## Core Rule

This is behavior-preserving body movement only. Do not change route4 record
shape, route-index validation behavior, record ordering, optionality, or route6
publication source selection.

## Read First

- `ideas/open/524_bir_route4_publication_body_extraction.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- `.codex/skills/c4c-clang-tools/SKILL.md`

## Current Targets

- Primary source: `src/backend/bir/bir.cpp`
- Public declarations: `src/backend/bir/bir.hpp`
- Likely new owner file: `src/backend/bir/bir_route4_publication.cpp`
- Build wiring: `src/backend/CMakeLists.txt`

## Non-Goals

- Do not move route4 public declarations out of `bir.hpp`.
- Do not move route-index facade bodies or validation records.
- Do not move route5, route6, or route7 bodies.
- Do not alter route6 publication source selection.
- Do not add semantic producer behavior or prepared contract behavior.
- Do not rewrite tests, expectations, unsupported markers, or diagnostics to
  make the move pass.

## Working Model

Route4 owns publication availability facts for current-block and block-entry
publication. The extraction should preserve the existing API surface while
moving cohesive implementation bodies and any strictly private local helpers
needed by those bodies into a dedicated translation unit.

Use AST-backed queries before raw long-file reading. If `c4c-clang-tool` or
`c4c-clang-tool-ccdb` is unavailable, record that in `todo.md` and fall back to
targeted text inspection with `rg`.

## Execution Rules

- Keep each code slice behavior-preserving and reviewable.
- Prefer moving existing bodies intact before cleanup.
- Add private declarations only when a moved helper needs cross-file linkage.
- Preserve namespaces, signatures, names, enum values, and diagnostics.
- Run build proof before focused tests.
- Use `test_after.log` for executor proof unless the supervisor delegates a
  different artifact.
- If route4 extraction exposes a missing semantic producer or route6 policy
  problem, stop and report it instead of expanding this idea.

## Ordered Steps

### Step 1: Map Route4 Ownership And Dependencies

Goal: identify the exact route4 publication bodies, local helpers, callers, and
route6 or facade consumers before moving code.

Actions:

- Load `.codex/skills/c4c-clang-tools/SKILL.md`.
- Confirm `c4c-clang-tool` and `c4c-clang-tool-ccdb` availability.
- Query route4 publication symbols in `src/backend/bir/bir.cpp` and
  declarations in `src/backend/bir/bir.hpp`.
- Map direct callers/callees for:
  - `route4_build_publication_availability_index`
  - `route4_validate_current_block_publication_reference`
  - block-entry publication validation helpers with route4 ownership
- Identify which helpers can move unchanged and which must remain with facade
  or other routes.
- Record the selected movement boundary and proof command in `todo.md`.

Completion Check:

- `todo.md` names the concrete route4 symbols to move, helpers to keep in
  place, expected new file, and narrow proof command.

### Step 2: Extract Route4 Bodies

Goal: move current-block and block-entry publication implementation bodies into
a route4 owner file without changing behavior.

Actions:

- Add `src/backend/bir/bir_route4_publication.cpp` only if the Step 1 boundary
  confirms it is the right owner.
- Move the selected route4 implementation bodies from `bir.cpp`.
- Move only strictly private helpers required by those bodies.
- Keep public route4 declarations in `bir.hpp`.
- Add or adjust private declarations only as needed for linkage.
- Update build wiring in `src/backend/CMakeLists.txt`.

Completion Check:

- The project compiles far enough to prove the moved symbols are linked once
  and no public route4 declarations changed.

### Step 3: Prove Publication Behavior Is Unchanged

Goal: prove route4 publication availability and downstream route6 consumption
still observe the same facts.

Actions:

- Run the delegated build command.
- Run the focused publication proof selected in Step 1. The expected narrow
  starting point is:
  `ctest --test-dir build -R '^backend_prepare_frame_stack_call_contract$' --output-on-failure`
- Include route6-consuming proof if the moved bodies are consumed through the
  route6 path in the selected subset.
- Record exact commands and results in `todo.md`.

Completion Check:

- Build proof is green.
- Focused route4 publication proof is green.
- Any route6-consuming proof requested by the supervisor is green.

### Step 4: Final Drift Check

Goal: ensure the slice remains a body extraction, not a semantic or facade
rewrite.

Actions:

- Inspect the diff for public API movement, route-index facade movement,
  route6 policy changes, test expectation changes, and diagnostic changes.
- Confirm no unsupported markers, allowlists, or expected output contracts were
  weakened.
- Update `todo.md` with final proof and remaining follow-up notes.

Completion Check:

- The diff matches the source idea scope and contains no testcase-shaped or
  semantic route drift.
