# AArch64 Module Phoenix Stage 2 Runbook

Status: Active
Source Idea: ideas/open/226_aarch64_module_phoenix_review_replacement_layout.md

## Purpose

Review the Stage 1 AArch64 module extraction evidence and define the Stage 3
replacement draft layout for direct prepared-BIR-to-MIR machine-node lowering.

## Goal

Produce a concrete review/layout document and a Stage 2 to Stage 3 handoff that
let the next phoenix stage start without re-deriving the architecture.

## Core Rule

This stage is design review and replacement layout only. Do not edit
implementation files, do not draft replacement component contents, and do not
delete or disable `src/backend/mir/aarch64/module/module.cpp`. It remains
compiled legacy evidence until replacement implementation plus build wiring
exist in a later stage.

## Read First

- `.codex/skills/phoenix-rebuild/SKILL.md`
- `ideas/open/226_aarch64_module_phoenix_review_replacement_layout.md`
- `src/backend/mir/aarch64/module/module.cpp.md`
- `src/backend/mir/aarch64/module/module.hpp.md`
- `src/backend/mir/aarch64/module/module.md`

## Current Targets

- Stage 2 review/layout artifact under `src/backend/mir/aarch64/module/`
- Stage 2 to Stage 3 handoff artifact under `src/backend/mir/aarch64/module/`
- Existing Stage 1 extraction evidence only as review input

## Non-Goals

- No real `.cpp` / `.hpp` implementation edits.
- No dispatcher rewiring or build-system rewiring.
- No per-file replacement draft contents.
- No test expectation changes, unsupported downgrades, or capability claims.
- No broad work in object encoding, linker behavior, x86, RISC-V, or unrelated
  AArch64 instruction coverage.
- No redesign that simply preserves the legacy module emitter under new names.

## Working Model

The legacy module emitter is behavior evidence, not the replacement shape. The
review should reconstruct the current prepared-BIR, prepared authority record,
codegen record, public assembly, and API exposure flow, then define a new
direct lowering layout around MIR machine-node production.

The layout must explicitly judge the parent 224 failure family:

- common MIR carrier confusion
- flat target-local vectors
- cached display strings
- target/common printer boundary drift

## Execution Rules

- Preserve source idea intent; do not edit the idea unless lifecycle metadata
  genuinely needs to change.
- Keep Stage 1 evidence reviewable. If the extraction is weak or misleading,
  state the required repair before Stage 3 can trust it.
- Classify legacy special cases as core lowering, optional fast path, legacy
  compatibility, or overfit to reject.
- Define responsibility seams by dependency direction and owned knowledge, not
  by line ranges or current helper clusters.
- Name the exact mandatory `.cpp.md` / `.hpp.md` replacement draft files that
  Stage 3 must fill.
- The handoff must identify trustworthy extraction artifacts, artifacts that
  require correction first, mandatory draft files, and route constraints.

## Steps

### Step 1: Review Stage 1 Evidence Shape

Goal: Decide whether the extracted artifact set is trustworthy enough to drive
replacement layout work.

Primary targets:

- `src/backend/mir/aarch64/module/module.cpp.md`
- `src/backend/mir/aarch64/module/module.hpp.md`
- `src/backend/mir/aarch64/module/module.md`

Actions:

- Inspect all Stage 1 markdown evidence.
- Check that the artifacts cover the legacy module `.cpp`, the directory index
  `.hpp`, and the directory-level subsystem index.
- Identify any missing, overlong, misleading, undercompressed, or wrongly
  organized evidence.
- Record required extraction repairs, if any, before trusting later stages.
- Preserve the correction that `module.cpp` remains compiled legacy evidence
  until replacement implementation plus build wiring exist.

Completion check:

- The executor can state which Stage 1 artifacts are trustworthy, which need
  repair, and whether Stage 2 can continue without a Stage 1 repair loop.

### Step 2: Reconstruct Current Subsystem Shape

Goal: Explain how the current AArch64 module emitter works without adopting
its helper boundaries as the new design.

Actions:

- Map current entry points, data inputs, output records, and public API
  exposure from the evidence.
- Trace dependencies on prepared BIR, prepared authority records, codegen
  records, public assembly, and MIR/module emission.
- Identify responsibility buckets and hidden cross-helper state.
- Classify special cases as core lowering, optional fast path, legacy
  compatibility, or overfit to reject.

Completion check:

- The review/layout artifact explains the current subsystem shape well enough
  to support a replacement layout decision.

### Step 3: Define Replacement Architecture Layout

Goal: Define direct prepared-BIR-to-MIR machine-node lowering seams for the
Stage 3 draft set.

Actions:

- Define replacement responsibilities by ownership and dependency direction.
- Specify which components own module dispatch, function traversal, value or
  operand resolution, instruction lowering, branch/control lowering, call
  lowering, public assembly bridging, and compatibility behavior if needed.
- State what each component may know, what it must not know, and what it
  outputs.
- Explain how the layout addresses the parent 224 failure family.
- Reject layouts that keep a single catch-all module emitter under renamed
  helpers.

Completion check:

- The review/layout artifact contains a concrete replacement architecture and
  an explicit parent-224 judgment.

### Step 4: Declare Stage 3 Draft Artifact Map

Goal: Make the Stage 3 source idea executable by naming the exact draft files
that must be produced.

Actions:

- Declare every mandatory replacement `.cpp.md` and `.hpp.md` draft artifact.
- Keep the phoenix header policy visible: one non-helper index `.hpp` per
  replacement directory; `helper.hpp` is the only allowed exception.
- Name any required replacement directory-level index `.md`.
- State whether any Stage 1 extraction repair must happen before a specific
  draft file is trustworthy.

Completion check:

- A Stage 3 executor can list all required draft files without inventing new
  layout scope.

### Step 5: Write Stage 2 To Stage 3 Handoff

Goal: Produce the explicit intake contract for the next phoenix stage.

Actions:

- Write a handoff artifact that identifies trustworthy evidence, required
  extraction corrections, mandatory draft files, and route constraints.
- Include non-goals and rejection signals for Stage 3 drift.
- Make any sequencing dependencies explicit.

Completion check:

- The handoff exists and is concrete enough that Stage 3 can start without
  re-reading the entire legacy implementation to decide its architecture.

### Step 6: Final Stage 2 Acceptance Check

Goal: Confirm the source idea completion signal is met before requesting
closure.

Actions:

- Verify the review/layout document exists and covers current shape,
  extraction quality, replacement layout, parent 224 judgment, and Stage 3
  draft map.
- Verify the handoff document exists and matches the layout.
- Verify no implementation files, build wiring, tests, or expectations were
  changed as part of Stage 2.
- Record proof notes in `todo.md`.

Completion check:

- The active idea is ready for supervisor review and possible closure decision.
