# AArch64 ARM-Reference Layout Contract Runbook

Status: Active
Source Idea: ideas/open/205_aarch64_arm_reference_layout_contract.md

## Purpose

Define the AArch64 backend layout and ownership contract before any further
implementation grows around convenient files or testcase-shaped gaps.

## Goal

Produce an accepted markdown responsibility ledger under
`src/backend/mir/aarch64/` that maps the ARM reference layout vocabulary onto
the current C++ BIR / `PreparedBirModule` pipeline, including explicit
BIR/prepared carrier status and split-out gap ideas when required.

## Core Rule

This plan is a layout and ownership contract only. Do not add instruction
selection, assembly emission, object writing, linker behavior, or target-local
workarounds for missing BIR/prepared facts.

## Read First

- `ideas/open/205_aarch64_arm_reference_layout_contract.md`
- `ref/claudes-c-compiler/src/backend/arm/README.md`
- `ref/claudes-c-compiler/src/backend/arm/codegen/README.md`
- `ref/claudes-c-compiler/src/backend/arm/assembler/README.md`
- `ref/claudes-c-compiler/src/backend/arm/linker/README.md`
- `src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md`
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- `src/backend/mir/aarch64/CLASSIFICATION_INDEX.md`
- Current live files under `src/backend/mir/aarch64/api/`,
  `src/backend/mir/aarch64/abi/`, and `src/backend/mir/aarch64/module/`.

## Current Targets

- Markdown responsibility-boundary updates under `src/backend/mir/aarch64/`.
- Any necessary new `ideas/open/` BIR/prepared gap ideas discovered by the
  layout review.
- Existing AArch64 live C++ files only for inspection unless a later source
  idea explicitly authorizes movement or implementation.

## Non-Goals

- Do not add scalar, memory, call, branch, return, or data instruction
  selection.
- Do not emit assembly text, assemble objects, write object files, or link
  binaries.
- Do not move large live code or expand `module/module.cpp` /
  `module/module.hpp` to make behavior progress.
- Do not copy the Rust ARM reference layout mechanically without mapping it to
  C++ `PreparedBirModule` facts.
- Do not recover backend semantics from rendered names, printed BIR, old
  markdown-derived shapes, assembly strings, parser operands, or testcase
  spellings.
- Do not weaken, skip, or reclassify tests to make a misplaced feature look
  acceptable.

## Working Model

- Treat BIR as the semantic MIR source and `PreparedBirModule` as the
  supplemental backend fact carrier.
- Every planned AArch64 feature family needs an owner, consumed BIR facts,
  supplemental prepared facts, target-local record type, deferred status when
  applicable, and first allowed implementation idea.
- Current `api/`, `abi/`, and `module/` C++ files are part of the live boundary
  to classify, not a default destination for all future records.
- Missing BIR/prepared facts must become explicit gap ideas instead of
  AArch64-local shortcuts.

## Execution Rules

- Keep source-idea edits unnecessary unless durable intent proves wrong.
- Prefer markdown-only slices until the ledger is accepted.
- When a BIR/prepared carrier is insufficient, create a separate
  `ideas/open/` gap idea with concrete reviewer reject signals.
- Keep proof appropriate to docs/lifecycle work: markdown inspection for
  docs-only changes, build/test proof only if code or build files change.
- Preserve unrelated dirty files and transient `review/` artifacts.

## Steps

### Step 1: Audit Current AArch64 And ARM Reference Surfaces

Goal: establish the feature-family inventory and the current AArch64 files that
must be mapped.

Primary target: `ref/claudes-c-compiler/src/backend/arm/` and
`src/backend/mir/aarch64/`.

Actions:

- List ARM reference families across top-level, `codegen/`, `assembler/`, and
  `linker/`.
- List current AArch64 markdown and live C++ surfaces, including `api/`,
  `abi/`, and `module/`.
- Compare existing AArch64 ledgers/contracts against the source idea's required
  family list.
- Record in `todo.md` the exact markdown file or files that should own the
  layout ledger update.

Completion check:

- `todo.md` names the ledger target, the reference families to cover, and the
  narrow proof command or inspection method for the first documentation slice.

### Step 2: Draft The Layout Ownership Ledger

Goal: document the AArch64 directory/file ownership contract for every required
feature family.

Primary target: markdown under `src/backend/mir/aarch64/`.

Actions:

- Map each ARM reference feature family to its intended AArch64 owner:
  `api/`, `abi/`, `module/`, `codegen/`, `assembler/`, `linker/`, or deferred
  contract.
- Include public prepared-module entry, ABI/AAPCS64, MIR containers, operands,
  registers, frame/prologue/epilogue, branches, calls, memory, globals/data,
  moves/spills/reloads, scalar/floating/special operations, assembler, encoder,
  object writer, binary utils, and linker surfaces.
- Decide which current `module/` records remain module-owned and which belong
  to later `abi/`, `codegen/`, `assembler/`, or deferred contracts.
- State first allowed implementation idea for each family; mark deferred
  families explicitly.

Completion check:

- The markdown ledger is more than a directory listing: every family has an
  owning file or deferred status plus a first allowed implementation route.

### Step 3: Add BIR/Prepared Carrier Checklist

Goal: make the input facts behind each layout decision reviewable.

Primary target: the same ledger or companion markdown under
`src/backend/mir/aarch64/`.

Actions:

- For each feature family, classify required facts as present in BIR, present
  only after shared preparation, missing from both, or intentionally deferred.
- Name the relevant BIR or `PreparedBirModule` carrier where present.
- For missing or ambiguous carriers, explain why target-local code must not
  work around the gap.
- Keep volatility/address-space, AAPCS64 call completeness, assembler/object,
  linker, inline asm, f128/i128, atomics, and intrinsics decisions explicit.

Completion check:

- A reviewer can trace every family from ARM reference vocabulary to BIR facts,
  prepared facts, target record, owner, and gap/deferred status.

### Step 4: Split Required BIR/Prepared Gap Ideas

Goal: ensure discovered carrier gaps are durable source ideas, not buried
inside the AArch64 ledger.

Primary target: `ideas/open/`.

Actions:

- Create one or more separate BIR/prepared gap ideas for every carrier that is
  required before the next implementation wave.
- Include goal, scope, out-of-scope work, acceptance criteria, and concrete
  reviewer reject signals for each new gap idea.
- If no required gap exists for the next implementation wave, make the markdown
  output say so explicitly and identify any deferred gaps.

Completion check:

- Every required missing carrier has a separate `ideas/open/` source idea, or
  the ledger explicitly states that no required BIR/prepared gap was found.

### Step 5: Consolidate And Validate The Contract

Goal: leave the lifecycle ready for supervisor review and later implementation
planning.

Primary target: updated markdown responsibility boundaries plus lifecycle
state.

Actions:

- Re-read the source idea acceptance criteria and reviewer reject signals.
- Verify the ledger covers all required families and the current `api/`,
  `abi/`, and `module/` roles.
- Verify no implementation behavior, assembly/object/linker behavior, or test
  downgrades were added.
- Run docs-only inspection proof, or build/test proof if any non-doc files were
  changed by an executor.
- Record final proof and remaining deferred items in `todo.md`.

Completion check:

- The source idea's required deliverables are present, reviewable, and ready
  for the supervisor to decide whether the active idea can close.
