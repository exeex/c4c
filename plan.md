# Draft Replacement X86 Codegen Interfaces For Phoenix Rebuild

Status: Active
Source Idea: ideas/open/80_draft_replacement_x86_codegen_interfaces_for_phoenix_rebuild.md
Activated after closing: ideas/closed/79_review_extracted_x86_codegen_subsystem_for_phoenix_rebuild.md

## Purpose

Turn the reviewed stage-2 rebuild plan into a complete per-file markdown draft
set under `docs/backend/x86_codegen_rebuild/` so stage 4 can convert an
explicit replacement contract instead of improvising implementation seams.

## Goal

Produce every stage-2-declared replacement `.cpp.md` / `.hpp.md`, the
directory-level draft indexes, and `docs/backend/x86_codegen_rebuild/review.md`
without changing the reviewed layout contract or touching live implementation.

## Core Rule

Do not convert any draft into real `.cpp` / `.hpp` code during this runbook.
Stage 3 owns draft generation and draft review only.

## Read First

- `ideas/open/80_draft_replacement_x86_codegen_interfaces_for_phoenix_rebuild.md`
- `ideas/closed/79_review_extracted_x86_codegen_subsystem_for_phoenix_rebuild.md`
- `docs/backend/x86_codegen_rebuild_plan.md`
- `docs/backend/x86_codegen_rebuild_handoff.md`
- `docs/backend/x86_codegen_subsystem.md`

## Scope

- the full replacement draft tree under `docs/backend/x86_codegen_rebuild/`
- the exact manifest declared in
  `docs/backend/x86_codegen_rebuild_plan.md`
- the intake constraints and trust boundaries from
  `docs/backend/x86_codegen_rebuild_handoff.md`
- per-file ownership direction for canonical lowering seams, prepared fast
  paths, debug surfaces, module/data emission, and ABI support
- the explicit review artifact at `docs/backend/x86_codegen_rebuild/review.md`

## Non-Goals

- converting drafts into `src/backend/mir/x86/codegen/`
- changing the reviewed stage-2 layout without a stage-2 repair
- deleting or retiring legacy source files
- proving final runtime correctness for the rebuilt subsystem

## Working Model

- treat `docs/backend/x86_codegen_rebuild_plan.md` as the manifest contract
- treat `docs/backend/x86_codegen_rebuild_handoff.md` as the route and trust
  contract
- keep each draft file explicit about owned inputs, owned outputs, allowed
  indirect queries, forbidden knowledge, and role classification
- keep prepared routes as bounded consumers of shared seams rather than a
  parallel lowering stack
- keep the draft tree reviewable as a coherent ownership map, not a prose dump

## Execution Rules

- prefer `manifest coverage -> core seam drafts -> prepared/debug drafts ->
  draft review`
- do not silently rename, merge, split, or drop stage-2-declared artifacts
- if a needed file seems wrong for the draft tree, record the conflict in
  `todo.md` and stop for stage-2 repair instead of freelancing the layout
- keep dependency direction explicit at the file level
- reserve `docs/backend/x86_codegen_rebuild/review.md` for an actual review of
  coherence, not a duplicate manifest list

## Step 1: Materialize Manifest Coverage And Directory Skeleton

Goal: create the stage-2-declared draft tree structure and top-level index
artifacts without yet filling in every per-file contract.

Primary targets:

- `docs/backend/x86_codegen_rebuild/index.md`
- `docs/backend/x86_codegen_rebuild/layout.md`
- directory structure under `docs/backend/x86_codegen_rebuild/`

Actions:

- create the directory/index artifacts required by the stage-2 manifest
- create placeholder draft files for every required `.cpp.md` and `.hpp.md`
  path so manifest coverage is explicit from the start
- organize the tree by the reviewed ownership buckets: `api`, `core`, `abi`,
  `module`, `lowering`, `prepared`, and `debug`
- verify the draft tree matches the stage-2 manifest exactly

Completion check:

- every required draft path exists under `docs/backend/x86_codegen_rebuild/`
  and the directory-level indexes describe the reviewed ownership layout

## Step 2: Draft Canonical Entry, Core, ABI, And Module Contracts

Goal: write the per-file contracts for the entry, shared core, ABI, and module
layers that the lowering and prepared routes must consume.

Primary targets:

- `docs/backend/x86_codegen_rebuild/api/`
- `docs/backend/x86_codegen_rebuild/core/`
- `docs/backend/x86_codegen_rebuild/abi/`
- `docs/backend/x86_codegen_rebuild/module/`

Actions:

- draft `x86_codegen_api`, `x86_codegen_types`, `x86_codegen_output`, target
  ABI, module emission, and module data emission contracts
- make the public entrypoints, shared data flow, and ABI facts explicit
  without reintroducing the old `x86_codegen.hpp` mixed-responsibility shape
- state which module and output responsibilities are canonical seams versus
  compatibility or indirect-query surfaces

Completion check:

- the entry, core, ABI, and module draft files form a coherent shared seam set
  that later lowering and prepared drafts can reference directly

## Step 3: Draft Canonical Lowering Family Contracts

Goal: write the replacement lowering-file contracts so canonical lowering
ownership is explicit before prepared adapters are described.

Primary targets:

- `docs/backend/x86_codegen_rebuild/lowering/`

Actions:

- draft the frame, call, return, memory, comparison, scalar, float, and
  atomics/intrinsics `.hpp.md` / `.cpp.md` pairs
- define owned inputs, outputs, and allowed queries for each lowering family
- keep cross-family coordination explicit and bounded so the draft set does
  not hide the same couplings the rebuild is meant to remove

Completion check:

- the lowering draft set names clear canonical owners for the major x86 codegen
  families and makes their boundaries legible without live-source re-reading

## Step 4: Draft Prepared And Debug Adapter Contracts

Goal: describe the prepared-route and debug surfaces as thin consumers of the
canonical seams rather than parallel lowering owners.

Primary targets:

- `docs/backend/x86_codegen_rebuild/prepared/`
- `docs/backend/x86_codegen_rebuild/debug/`

Actions:

- draft the prepared query context, fast-path dispatch, fast-path operands,
  and prepared route debug contracts
- show how prepared routes consume canonical call, frame, memory, comparison,
  and output seams instead of reconstructing those policies locally
- mark any remaining compatibility facts explicitly so they cannot masquerade
  as general lowering ownership

Completion check:

- the prepared/debug draft files read as bounded adapters over the canonical
  seams rather than a second subsystem hidden in markdown

## Step 5: Review Draft Coherence And Readiness

Goal: review the full draft set for manifest completeness, ownership
coherence, and stage-4 handoff readiness.

Primary targets:

- `docs/backend/x86_codegen_rebuild/review.md`
- `docs/backend/x86_codegen_rebuild/`

Actions:

- review whether the draft set matches the exact manifest from stage 2
- review whether dependency direction is legible across entry, lowering,
  prepared, debug, and module layers
- record whether prepared routes remain consumers of canonical seams
- note any stage-2 contract conflicts or unresolved compatibility pressure

Completion check:

- `docs/backend/x86_codegen_rebuild/review.md` confirms the draft set is
  coherent enough to drive stage-4 implementation conversion
