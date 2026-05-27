# AArch64 Duplicate Prepared Authority Audit

## Goal

Audit seven large AArch64 codegen files for duplicated BIR/prealloc/shared
prepared authority, then split seven follow-up ideas that each repair one file
family by consuming existing shared facts, adding missing shared authority, or
retiring legacy target-local recovery.

## Why This Exists

`src/backend/mir/aarch64/codegen` remains much larger than the reference ARM
layout under `ref/claudes-c-compiler/src/backend/arm/codegen`. Some of the
remaining size may be legitimate target emission, but there is a strong signal
that AArch64 still duplicates work already represented in BIR or shared
prealloc/prepared facts.

Examples to investigate include shared edge-publication source producers,
prepared scalar publication plans, prepared store-source publication plans,
prepared block-entry publications, call plans, value homes, and producer
indexes that AArch64 may still rederive through same-block scans or
target-local fallback paths.

This idea is an audit and follow-up splitting slice. It should not directly
rewrite the seven files.

## Files To Audit

- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`

## In Scope

- Compare each file against existing BIR/prealloc/shared prepared authority.
- Compare each file's owner responsibilities against the reference ARM codegen
  layout, especially `emit.rs`, `calls.rs`, `memory.rs`, `alu.rs`, and
  `comparison.rs`.
- Identify duplicated local producer scans, source/home recovery, publication
  planning, edge-copy facts, call source decisions, and same-block fallback
  paths.
- Classify each suspicious helper as:
  - `consume-shared`: AArch64 should use an existing shared fact.
  - `missing-shared-field`: shared authority exists but lacks a field/query.
  - `target-emission`: AArch64 should keep it as register/scratch/addressing
    or instruction spelling logic.
  - `legacy-fallback`: old local recovery should be retired or fail-closed.
  - `needs-more-evidence`: requires a narrow proof before choosing a route.
- Produce exactly seven follow-up ideas, one for each audited file, with stable
  numbering and owned repair scope.
- Each follow-up idea should name the shared facts it intends to consume or the
  missing shared authority it must add.

## Out Of Scope

- Direct implementation edits in the audited files.
- Mechanical file merging or build metadata cleanup.
- Broad AArch64 codegen rewrites.
- Reopening already-closed fold-back cleanup unless the audit finds a concrete
  duplicate-authority bug.
- Treating line count reduction as success without removing duplicated
  authority.

## Acceptance Criteria

- A durable audit table exists for all seven files.
- The audit distinguishes existing shared facts from genuinely missing shared
  authority.
- Seven numbered follow-up ideas are created under `ideas/open/`, one per
  audited file.
- Follow-up ideas separate semantic authority repair from target-local
  emission cleanup.
- The final close note states which files appear to duplicate existing
  BIR/prealloc authority and which are mostly target-emission residue.

## Reviewer Reject Signals

- A patch edits implementation while claiming to be audit-only.
- A patch says "move to BIR" without naming the current duplicated helper or
  the required shared fact.
- A patch treats every producer lookup as wrong without checking whether it is
  target-local hazard/emission logic.
- A patch produces vague follow-up ideas that do not own one audited file each.
- A patch ignores the reference ARM layout or the existing prepared authority
  in `src/backend/prealloc`.
