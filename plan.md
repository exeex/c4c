# Extract Full X86 Backend Subsystem To Markdown For Phoenix Rebuild

Status: Active
Source Idea: ideas/open/82_extract_full_x86_backend_subsystem_to_markdown_for_phoenix_rebuild.md
Supersedes: ideas/open/59_cfg_contract_consumption_for_short_circuit_and_guard_chain.md
Activated from: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md

## Purpose

Create the missing stage-1 Phoenix evidence for the full
`src/backend/mir/x86/` subsystem so future teardown and replacement work can
cover the root dispatcher, assembler, linker, and accepted codegen rebuild as
one explicit ownership graph.

## Goal

Replace ad hoc live-tree deletion and codegen-only Phoenix assumptions with one
reviewable extraction package for the whole `x86` subsystem.

## Core Rule

Do not count deletion or restructuring under `src/backend/mir/x86/` as Phoenix
progress until the matching legacy behavior has a compressed markdown evidence
artifact.

## Read First

- `ideas/open/82_extract_full_x86_backend_subsystem_to_markdown_for_phoenix_rebuild.md`
- `.codex/skills/phoenix-rebuild/SKILL.md`
- `docs/backend/x86_codegen_legacy/index.md`
- `docs/backend/x86_codegen_rebuild/index.md`
- `docs/backend/x86_codegen_rebuild_handoff.md`

## Scope

- `src/backend/mir/x86/mod.cpp`
- `src/backend/mir/x86/assembler/`
- `src/backend/mir/x86/linker/`
- accepted prior Phoenix evidence for `src/backend/mir/x86/codegen/`

## Non-Goals

- redefining the replacement architecture before stage 2
- converting replacement drafts into live implementation
- counting the already-open codegen stage-4 idea as sufficient evidence for the
  rest of `src/backend/mir/x86/`
- deleting more live legacy code before matching extraction artifacts exist

## Working Model

- treat the existing `docs/backend/x86_codegen_legacy/` package as accepted
  stage-1 evidence for the codegen subtree
- treat the missing root, assembler, and linker extraction as the blocker for a
  whole-subsystem Phoenix route
- keep one accepted non-helper directory index header per directory:
  `x86_codegen.hpp`, `assembler/mod.hpp`, `assembler/encoder/mod.hpp`, and
  `linker/mod.hpp`
- treat `parser.hpp` and similar local headers as helper or compatibility
  content, not as second directory indexes

## Execution Rules

- use compressed extraction rather than source dumps
- keep short fenced `cpp` blocks only for essential surfaces
- classify each notable responsibility seam and special case as `core
  lowering`, `optional fast path`, `legacy compatibility`, or `overfit to
  reject`
- if a deleted legacy `.cpp` lacks a matching extraction artifact, restore it
  or stop the teardown route before later stages proceed
- preserve `todo.md` as the packet log; do not rewrite this runbook for routine
  extraction progress

## Step 1: Freeze The Whole-Subsystem Extraction Inventory

Goal: confirm the exact in-scope legacy source list, accepted directory-index
headers, and already-satisfied codegen evidence before extracting anything new.

Primary targets:

- `src/backend/mir/x86/mod.cpp`
- `src/backend/mir/x86/assembler/`
- `src/backend/mir/x86/linker/`
- `docs/backend/x86_codegen_legacy/index.md`

Actions:

- enumerate every in-scope legacy `.cpp` that still needs a new companion under
  `docs/backend/x86_subsystem_legacy/`
- confirm the accepted directory-index headers for each directory and record
  why any extra header is helper or compatibility-only rather than a second
  index
- confirm the existing `docs/backend/x86_codegen_legacy/` package is the
  accepted evidence for the codegen subtree and does not need regeneration in
  this step
- record any live-tree deletions that currently lack matching extraction
  evidence

Completion check:

- the stage-1 artifact map is explicit, directory-index ownership is explicit,
  and every currently deleted in-scope legacy `.cpp` is either scheduled for
  extraction from legacy evidence or flagged as a blocker

## Step 2: Extract The Missing Root, Assembler, And Linker Legacy Sources

Goal: create the missing per-file evidence for the non-codegen parts of the
`x86` subsystem.

Primary targets:

- `docs/backend/x86_subsystem_legacy/mod.cpp.md`
- `docs/backend/x86_subsystem_legacy/assembler/*.md`
- `docs/backend/x86_subsystem_legacy/assembler/encoder/*.md`
- `docs/backend/x86_subsystem_legacy/linker/*.md`

Actions:

- write one markdown companion for every in-scope legacy `.cpp` in the root,
  assembler, assembler encoder, and linker directories
- write one markdown companion for each accepted directory-index header:
  `assembler/mod.hpp`, `assembler/encoder/mod.hpp`, and `linker/mod.hpp`
- capture the real entry points, hidden dependencies, state handoff, and
  special-case buckets for each file
- avoid mechanically copying the source; summarize non-essential internals in
  prose

Completion check:

- every missing non-codegen legacy `.cpp` and accepted directory-index header
  has a reviewable companion artifact under `docs/backend/x86_subsystem_legacy/`

## Step 3: Build The Top-Level Whole-X86 Extraction Index

Goal: make one top-level index point at the complete whole-subsystem Phoenix
evidence package.

Primary targets:

- `docs/backend/x86_subsystem_legacy/index.md`
- `docs/backend/x86_codegen_legacy/index.md`

Actions:

- summarize the current subsystem ownership graph across root dispatch,
  assembler, linker, and codegen
- point the top-level index at every new root/assembler/linker artifact plus
  the accepted `docs/backend/x86_codegen_legacy/` package
- call out the major responsibility overlaps and cross-directory dependency
  directions that later stages must redesign
- explicitly classify the remaining live compatibility surface that the stage-4
  codegen rebuild does not yet cover

Completion check:

- `docs/backend/x86_subsystem_legacy/index.md` acts as the canonical entry
  point for the full extraction package and makes the whole-subsystem rebuild
  pressure reviewable at a glance

## Step 4: Reconcile Teardown State With The Extraction Evidence

Goal: ensure the current live-tree deletions under `src/backend/mir/x86/`
comply with the Phoenix teardown rule before later stages proceed.

Primary targets:

- current worktree deletions in `src/backend/mir/x86/mod.cpp`
- current worktree deletions under `src/backend/mir/x86/assembler/`
- current worktree deletions under `src/backend/mir/x86/linker/`

Actions:

- verify every deleted legacy `.cpp` now has matching extraction evidence
- if any deletion still lacks evidence, restore it or explicitly stop the
  teardown route instead of silently keeping the file removed
- update `todo.md` with the coverage state, blockers, and which directories are
  now extraction-complete

Completion check:

- no in-scope deleted legacy `.cpp` remains unsupported by stage-1 extraction
  evidence, and the whole-subsystem Phoenix route can advance honestly to stage
  2
