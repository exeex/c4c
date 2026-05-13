# AArch64 Markdown-First Backend Reconstruction Runbook

Status: Active
Source Idea: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Activated: 2026-05-13

## Purpose

Restart AArch64 backend work by converting the previous
`src/backend/mir/aarch64` implementation surface into reviewable markdown
artifacts before growing any new production `.cpp` code.

## Goal

Produce a classified markdown review of the old AArch64 route, define the
accepted backend entry contract against current BIR / `PreparedBirModule`
facts, and decide the first small follow-up implementation idea.

## Core Rule

Do not debug, patch, or expand old AArch64 `.cpp` implementation files while
the markdown extraction, classification index, and BIR/prepared interface
ledger are incomplete.

## Read First

- `ideas/open/203_aarch64_markdown_first_backend_reconstruction.md`
- `src/backend/mir/aarch64/`
- Current BIR and prepared-backend contracts around `PreparedBirModule`

## Current Targets

- Existing files under `src/backend/mir/aarch64`
- Markdown review artifacts for old codegen, assembler, linker, and module
  entry surfaces
- A backend interface ledger covering `PreparedBirModule`, raw `bir::Module`
  only if justified, structured semantic identity, and target-local MIR/asm
  needs

## Non-Goals

- Do not start broad AArch64 instruction selection, register allocation,
  assembler, or linker rewrites in this runbook.
- Do not add rendered-name recovery or string-based semantic lookup fallback.
- Do not weaken backend tests or reclassify supported cases as unsupported to
  claim progress.
- Do not reopen parser, sema, HIR, LIR, or BIR identity work unless a concrete
  missing carrier is discovered and split into a separate open idea.

## Working Model

- `ideas/open/203_aarch64_markdown_first_backend_reconstruction.md` is the
  durable source intent.
- This runbook is the active execution transcript.
- `todo.md` is the live packet state and should record routine progress.
- Any required missing BIR/prepared carrier becomes a separate `ideas/open/`
  initiative before AArch64 backend implementation continues.

## Execution Rules

- Preserve useful old AArch64 content as markdown review material, not as live
  production `.cpp`.
- Classify every old AArch64 file as one of: salvageable design note, obsolete
  route, binary-utils candidate, target-ABI candidate, assembler/linker
  candidate, or delete/defer.
- Prefer `PreparedBirModule` as the new backend entry unless a narrower raw BIR
  slice is explicitly justified in the ledger.
- Use structured ids already present in BIR/prealloc for semantic identity.
- Mark retained compatibility or deprecated-route notes with `legacy` or
  `deprecated`, plus owner, limitation, and removal condition.
- For code-changing packets, require build proof. Escalate to broader checks
  before accepting any milestone that changes backend build surfaces.

## Ordered Steps

### Step 1: Inventory Existing AArch64 Surface

Goal: enumerate the old backend implementation surface before changing it.

Primary target: `src/backend/mir/aarch64/`

Actions:

- Inspect the directory and list every existing `.cpp`, header, build, and
  support artifact under the AArch64 backend path.
- Group files by likely responsibility: top-level module entry, codegen,
  target ABI, assembler, linker, binary utilities, tests/helpers, or unknown.
- Identify which files are old production `.cpp` surfaces that must be removed
  from the live implementation tree during markdown extraction.

Completion check:

- `todo.md` records the inventory result and the next extraction target.
- No implementation edits are required for this step unless the executor packet
  explicitly owns the extraction.

### Step 2: Extract Old `.cpp` Surfaces To Markdown Artifacts

Goal: remove old production `.cpp` files from the live AArch64 implementation
surface while preserving reviewable design content.

Primary target: `src/backend/mir/aarch64/`

Actions:

- For each old `.cpp` file, create a markdown artifact that summarizes useful
  structure, assumptions, entry points, dependencies, and known failure risks.
- Remove the corresponding old `.cpp` from the live implementation surface once
  its relevant content has been abstracted.
- Keep extraction descriptive. Do not perform semantic repair in the markdown
  pass.
- Preserve or adjust build metadata only as needed so the tree no longer
  expects deleted old `.cpp` files.

Completion check:

- All pre-existing old `.cpp` files under `src/backend/mir/aarch64` are gone
  from the live tree.
- Their useful content is represented in `.md` review artifacts.
- A fresh build or compile proof covers the changed build surface.

### Step 3: Build The Markdown Classification Index

Goal: make the extracted AArch64 route cheap to review and reject stale paths.

Primary target: AArch64 markdown review index under the backend path or an
adjacent docs path chosen by the executor.

Actions:

- Create an index listing every extracted or inspected AArch64 artifact.
- Classify each artifact as salvageable design note, obsolete route,
  binary-utils candidate, target-ABI candidate, assembler/linker candidate, or
  delete/defer.
- Add `legacy` or `deprecated` notes where retained compatibility or stale
  routing remains relevant, including owner, limitation, and removal condition.
- Call out any artifact that should not influence the new BIR/prepared backend
  contract.

Completion check:

- The index can tell a reviewer which old pieces may inform future code and
  which pieces must not be revived.
- The index is linked or discoverable from the AArch64 backend path.

### Step 4: Define The Backend Entry Contract

Goal: specify the new AArch64 backend interface against current structured
BIR/prepared facts before code grows back.

Primary targets: current BIR, prepared-backend, and backend module-entry
interfaces.

Actions:

- Inspect the current `PreparedBirModule` contract and the raw `bir::Module`
  facts available to backends.
- Decide whether the new AArch64 route should consume `PreparedBirModule`, raw
  `bir::Module`, or a staged subset, and document why.
- Specify how semantic identity is carried through structured ids.
- Specify target-local MIR/asm structures needed before instruction selection
  or assembly generation resumes.
- Explicitly reject rendered-name recovery and string fallback routes.

Completion check:

- The contract document states the accepted entry type and its required facts.
- Any narrower-than-`PreparedBirModule` route has a concrete justification.

### Step 5: Produce The BIR/Prepared Gap Ledger

Goal: decide whether backend implementation may proceed or must first split a
BIR/prepared carrier initiative.

Primary target: the backend interface ledger created during Step 4.

Actions:

- Compare the accepted AArch64 contract against facts available in current BIR
  and `PreparedBirModule`.
- Record every required fact as present, missing, ambiguous, or deferred.
- For each missing or ambiguous required fact, decide whether it blocks AArch64
  backend work.
- If a blocking carrier is missing, stop backend implementation and create a
  separate `ideas/open/` BIR/prepared gap idea instead of working around it in
  target code.

Completion check:

- The ledger is complete enough to decide proceed versus split.
- No target-local workaround hides a missing structured BIR/prepared fact.

### Step 6: Select The First New Implementation Idea

Goal: end this reconstruction with a small code-oriented follow-up, not a broad
backend rewrite.

Primary target: `ideas/open/`

Actions:

- Use the markdown index, backend contract, and gap ledger to choose the first
  coherent implementation slice.
- Keep the slice small enough to prove the target interface before large
  compile/debug loops begin.
- Write the follow-up as a separate open idea if implementation work remains.
- Do not start the implementation inside this markdown-first runbook unless
  the supervisor explicitly switches lifecycle state.

Completion check:

- A next implementation idea exists or the ledger clearly explains why backend
  work is blocked.
- The current source idea can be evaluated against its acceptance criteria.

## Acceptance Check

This runbook is complete only when:

- The old AArch64 implementation surface has a markdown review index.
- All pre-existing old `.cpp` files under `src/backend/mir/aarch64` are gone
  from the live implementation tree.
- The backend entry contract is documented and avoids string-based semantic
  recovery.
- The BIR/prepared gap ledger supports a proceed-or-split decision.
- The next implementation idea is small, code-oriented, and starts after the
  markdown-first contract is accepted.
