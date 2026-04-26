# BIR Block Label Structured Identity Runbook

Status: Active
Source Idea: ideas/open/119_bir_block_label_structured_identity_for_assembler.md

## Purpose

Move BIR block labels and label references toward shared structured identity
while preserving existing dump spelling and leaving raw strings available as
compatibility fallbacks.

## Goal

BIR blocks, branch targets, phi incoming labels, and label-address operands can
carry `BlockLabelId` values backed by shared `TextId` spelling, and the BIR
printer can render through those ids when available.

## Core Rule

Structured label identity must remain text-stable: existing `--dump-bir`
output, including the trailing `:` on block labels, is the compatibility
contract unless the supervisor explicitly accepts a contract change.

## Read First

- ideas/open/119_bir_block_label_structured_identity_for_assembler.md
- src/shared/text_id_table.hpp
- src/backend/bir/bir.hpp
- src/backend/bir/bir_printer.cpp
- src/backend/bir/lir_to_bir/module.cpp
- src/backend/bir/lir_to_bir/cfg.cpp
- src/backend/prealloc/prealloc.hpp
- src/backend/prealloc/out_of_ssa.cpp
- src/backend/prealloc/prepared_printer.cpp
- focused tests covering `--dump-bir`, branch labels, conditional branches,
  phi incoming labels, focus-block filtering, focus-value filtering, and
  label-address operands

## Current Targets

- `bir::Block::label`
- `bir::PhiIncoming::label`
- `bir::PhiObservation` incoming labels
- `bir::Terminator::target_label`
- `bir::Terminator::true_label`
- `bir::Terminator::false_label`
- `bir::MemoryAddress::BaseKind::Label` plus `base_name`
- BIR module or function ownership of shared `TextTable` / `BlockLabelTable`
- BIR dump rendering for block labels, branch targets, phi incoming labels,
  focused dump filters, and label-address operands
- Assembler-facing handoff notes for remaining raw-string label paths

## Non-Goals

- Do not remove raw label strings in this runbook.
- Do not change final label spelling conventions.
- Do not make label identity a prerequisite for LIR/BIR type legacy removal.
- Do not migrate MIR target codegen internals.
- Do not add testcase-shaped label shortcuts or weaken existing test
  contracts.

## Working Model

- `src/shared/text_id_table.hpp` already defines `BlockLabelId`,
  `kInvalidBlockLabel`, and `BlockLabelTable`.
- BIR currently stores label spelling as repeated strings across blocks,
  branches, phi incoming edges, and label-address memory operands.
- Prepared backend code already has `PreparedNameTables::block_labels`; the BIR
  identity path should reuse the shared id/spelling model rather than inventing
  another label authority.
- During the transition, every structured id must have a raw string fallback,
  and unresolved target ids should be reported as proof gaps instead of being
  fabricated.

## Execution Rules

- Attach label identity at the BIR module/function creation boundary before
  changing downstream consumers.
- Intern each block spelling once through the chosen `BlockLabelTable` owner,
  then resolve branch, phi, and label-address references against that table.
- Keep id fields beside existing strings until a later cleanup idea removes
  raw fallbacks.
- Make rendering prefer `BlockLabelId -> TextId -> spelling` only when the id
  is valid and resolvable; otherwise render the existing string.
- Keep branch target ids aligned with the corresponding block label id.
- Record unresolved or raw-string-only paths in `todo.md`; edit the source idea
  only if durable intent changes.
- Use `test_after.log` for executor proof unless the supervisor delegates a
  different artifact.

## Steps

### Step 1: Inventory Current BIR Label Authority

Goal: Establish the exact label surfaces and the first safe attachment point
before changing structs.

Primary target: `src/backend/bir/bir.hpp`,
`src/backend/bir/lir_to_bir/module.cpp`, `src/backend/bir/lir_to_bir/cfg.cpp`,
and current dump/filter tests.

Actions:
- Inspect every BIR raw label field and every constructor/lowering path that
  writes it.
- Identify whether label identity should live on `bir::Module`,
  `bir::Function`, or a small shared label context reachable from printer and
  later prepared/assembler handoff code.
- List existing prepared-backend use of `PreparedNameTables::block_labels` and
  decide how BIR ids should map into that path without changing MIR internals.
- Select the first code-changing packet and record the inventory in `todo.md`.

Completion check:
- `todo.md` names the chosen label-table owner, all raw label surfaces, and the
  first implementation packet with a focused proof command.

### Step 2: Add Structured Label Fields Beside Raw Strings

Goal: Introduce BIR label ids without changing behavior.

Primary target: `src/backend/bir/bir.hpp` and direct BIR construction helpers.

Actions:
- Add `BlockLabelId` fields beside block labels, phi incoming labels,
  branch/conditional branch target labels, and label-address memory operands.
- Keep all new fields defaulting to `kInvalidBlockLabel`.
- Add any minimal include or helper plumbing needed to use the shared
  `BlockLabelId` aliases in BIR.
- Do not remove or rename existing string fields.

Completion check:
- The project builds with default invalid ids, existing dump output is
  unchanged, and proof is recorded in `test_after.log`.

### Step 3: Intern Labels During LIR-To-BIR Lowering

Goal: Populate structured ids while preserving the raw string compatibility
path.

Primary target: `src/backend/bir/lir_to_bir/module.cpp` and related
LIR-to-BIR label helpers.

Actions:
- Intern each emitted BIR block label once through the chosen label table.
- Resolve branch and conditional branch target ids from the interned block
  labels.
- Resolve phi incoming ids and label-address memory operand ids from the same
  table.
- Treat unresolved targets as explicit proof gaps: keep the raw string fallback
  and record the unresolved path rather than assigning a misleading id.

Completion check:
- Branch, conditional branch, phi, and label-address BIR paths carry valid ids
  where the target block or label can be resolved, and focused dump text remains
  byte-stable.

### Step 4: Render Labels Through Structured Identity

Goal: Make the BIR printer prefer structured label spelling when ids are
available.

Primary target: `src/backend/bir/bir_printer.cpp`.

Actions:
- Add a small label render helper that resolves `BlockLabelId` through the
  chosen table and falls back to the existing string.
- Use the helper for block headers, `bir.br`, `bir.cond_br`, `bir.phi`,
  semantic phi observations, and label-address memory operands.
- Keep the trailing `:` formatting owned by the block-header render site.
- Include focused dump-filter cases in proof when filter behavior depends on
  label spelling.

Completion check:
- The printer renders labels through ids when possible, fallback strings still
  preserve output when ids are invalid, and focused dump tests remain stable.

### Step 5: Align Prepared Backend Handoff

Goal: Keep structured BIR labels compatible with the existing prepared/backend
name-table route.

Primary target: `src/backend/prealloc/prealloc.hpp`,
`src/backend/prealloc/out_of_ssa.cpp`, and prepared printer / handoff helpers.

Actions:
- Compare BIR label ids with `PreparedNameTables::block_labels` use.
- Reuse or translate shared spelling ids at the handoff boundary without
  forcing a MIR target-codegen migration.
- Avoid duplicate interning that makes equivalent block labels diverge by id
  within one module/function path.
- Record any remaining assembler-facing raw-string-only label dependency in
  `todo.md`.

Completion check:
- Prepared backend handoff can consume stable label identity or an explicit
  translation from BIR labels, with remaining raw-string dependencies
  inventoried.

### Step 6: Prove Compatibility And Handoff Remaining Work

Goal: Finish the runbook with stable output proof and clear follow-up state.

Primary target: focused BIR dump tests, prepared backend tests selected by the
supervisor, `todo.md`, and source idea handoff notes only if durable intent
changes.

Actions:
- Run the supervisor-delegated build and focused test subset after each
  code-changing packet.
- Include branch, conditional branch, phi incoming, focus-block filtering,
  focus-value filtering, and label-address coverage before treating the idea
  as acceptance-ready.
- Escalate to broader backend validation if the implementation touches shared
  prepared/backend paths beyond narrow label handoff.
- Inventory raw-string-only label paths that remain because fallback removal is
  deferred.

Completion check:
- Structured BIR label ids are available for the accepted paths, dump text is
  preserved, proof is recorded, and remaining raw-string fallback removal is
  explicitly deferred to a later idea.
