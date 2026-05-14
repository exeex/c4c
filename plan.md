# AArch64 Module Phoenix Stage 1 Extraction Runbook

Status: Active
Source Idea: ideas/open/225_aarch64_module_phoenix_extract_legacy_evidence.md
Supersedes Active Route: ideas/open/224_common_mir_container_and_target_printer_boundary.md

## Purpose

Start the AArch64 module phoenix rebuild by extracting the current module
emitter and module surface into compressed markdown evidence. Treat
`module.cpp` as legacy behavior evidence, not as the design to patch.

## Goal

Produce reviewable markdown evidence for the in-scope AArch64 module files and
the directory index so later phoenix stages can design direct
prepared-BIR-to-MIR machine-node lowering from stable artifacts.

## Core Rule

This stage extracts and classifies legacy evidence only. It must not draft the
replacement architecture, edit implementation behavior, or make tests weaker.

## Read First

- `.codex/skills/phoenix-rebuild/SKILL.md`
- `ideas/open/225_aarch64_module_phoenix_extract_legacy_evidence.md`
- `ideas/open/224_common_mir_container_and_target_printer_boundary.md`
- `src/backend/mir/aarch64/module/module.cpp`
- `src/backend/mir/aarch64/module/module.hpp`

## Current Targets And Scope

- `src/backend/mir/aarch64/module/module.cpp`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/module/module.cpp.md`
- `src/backend/mir/aarch64/module/module.hpp.md`
- `src/backend/mir/aarch64/module/module.md`

## Non-Goals

- Do not implement the replacement route in this stage.
- Do not design new real `.cpp` / `.hpp` files before stage 2.
- Do not change prepared authority semantics for frame, call, allocation,
  spill/reload, storage plan, data, or control-flow facts.
- Do not add broad new AArch64 instruction coverage.
- Do not downgrade tests, mark supported paths unsupported, or claim progress
  through expectation rewrites.

## Working Model

The extraction packet should use the phoenix script to produce one markdown
companion per in-scope legacy `.cpp` / `.hpp`, then produce the directory-level
index. Stage 1 keeps `module.cpp` in place as compiled legacy evidence because
`src/backend/CMakeLists.txt` still builds it and no replacement implementation
exists yet. Teardown belongs to a later phoenix stage that owns replacement
implementation plus build wiring.

## Execution Rules

- Keep routine executor progress in `todo.md`.
- Use script-driven extraction:
  `python .codex/skills/phoenix-rebuild/scripts/extract_legacy_to_markdown.py 'src/backend/mir/aarch64/module/module.cpp' 'src/backend/mir/aarch64/module/module.hpp'`
- Do not run phoenix cleanup for `module.cpp` in Stage 1. The cleanup command
  is deferred until a later phoenix stage provides replacement implementation
  and updates the build graph.
- Keep extraction compressed and reviewable; do not dump full source.
- Use short fenced `cpp` blocks only for essential surfaces.
- Classify fast paths and special cases as core lowering, optional fast path,
  legacy compatibility, or overfit to reject.
- Preserve the header invariant: one non-helper directory index `.hpp` in the
  module directory; `helper.hpp` is the only allowed exception.

## Steps

### Step 1: Verify Phoenix Extraction Scope

Goal: confirm the in-scope legacy source set and header invariant before
running extraction.

Primary targets:
- `src/backend/mir/aarch64/module/module.cpp`
- `src/backend/mir/aarch64/module/module.hpp`

Actions:
- Confirm the module directory contains exactly one non-helper `.hpp` and that
  `module.hpp` is the directory index surface.
- Confirm stage 1 owns only `module.cpp`, `module.hpp`, and markdown evidence
  for the module directory.
- Record any surprise extra module-surface files in `todo.md` for supervisor
  routing instead of silently expanding the stage.

Completion check:
- `todo.md` records that the scope and header invariant are clear, or records
  the exact lifecycle ambiguity blocking extraction.

### Step 2: Run Scripted Markdown Extraction

Goal: produce compressed per-file markdown evidence for the in-scope legacy
source set.

Primary targets:
- `src/backend/mir/aarch64/module/module.cpp.md`
- `src/backend/mir/aarch64/module/module.hpp.md`

Actions:
- Run the phoenix extraction script over `module.cpp` and `module.hpp`.
- Ensure the artifacts capture APIs, contracts, dependency directions, hidden
  dependencies, responsibility buckets, and special-case classifications.
- Keep code excerpts short and limited to essential surfaces.

Completion check:
- Both per-file markdown companions exist and are compressed enough for review.

### Step 3: Write The Module Directory Index

Goal: connect the extracted evidence into one directory-level module index.

Primary target:
- `src/backend/mir/aarch64/module/module.md`

Actions:
- Summarize the current module emitter responsibilities and module surface.
- Point at every per-file extraction artifact.
- Record how the extracted module scope relates to public AArch64 assembly,
  `api/api.hpp`, codegen records, and prepared BIR inputs.
- Call out which current pieces are core lowering, optional fast path, legacy
  compatibility, or overfit to reject.

Completion check:
- The directory index exists, points at the full artifact set, and lets a
  reviewer understand the extracted scope without reopening source as the
  primary reference.

### Step 4: Check Build-Preserving Stage 1 Close Readiness

Goal: make the extraction set ready for supervisor/reviewer acceptance while
preserving the current build.

Primary targets:
- `src/backend/mir/aarch64/module/module.cpp.md`
- `src/backend/mir/aarch64/module/module.hpp.md`
- `src/backend/mir/aarch64/module/module.md`
- `src/backend/mir/aarch64/module/module.cpp`
- `todo.md`

Actions:
- Verify every in-scope source has the required markdown companion.
- Verify the one non-helper `.hpp` rule still holds.
- Verify the directory index points at the complete artifact set.
- Confirm `module.cpp` remains available to the build as legacy evidence; do
  not delete it, remove it from CMake, or replace it in this stage.
- Record any extraction limitations, handoff notes, or follow-up repair needs
  in `todo.md`.

Completion check:
- Stage 1 satisfies the source idea completion signal without breaking build
  wiring, or `todo.md` records the exact blocker preventing activation of
  stage 2.

### Step 5: Hand Off Deferred Teardown To Stage 2

Goal: leave a clean boundary for the later phoenix stage that will replace the
legacy module implementation.

Primary targets:
- `src/backend/mir/aarch64/module/module.md`
- `src/backend/mir/aarch64/module/module.cpp`
- `todo.md`

Actions:
- Ensure `todo.md` states that `module.cpp` is legacy evidence only, not a
  replacement design target.
- Record that physical teardown is deferred until replacement implementation
  and build-system wiring are ready.
- Do not draft replacement architecture or edit implementation files in this
  stage.

Completion check:
- A new agent can start the next phoenix stage from the markdown evidence and
  knows that deleting or disconnecting `module.cpp` is not accepted until the
  replacement stage owns build-preserving wiring.
