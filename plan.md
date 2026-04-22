# Review Extracted X86 Codegen Subsystem For Phoenix Rebuild

Status: Active
Source Idea: ideas/open/79_review_extracted_x86_codegen_subsystem_for_phoenix_rebuild.md
Activated after closing: ideas/closed/78_extract_x86_codegen_subsystem_to_markdown_for_phoenix_rebuild.md

## Purpose

Turn the stage-1 extraction set into a reviewed redesign input by checking
whether the extracted subsystem model is actually truthful, compressed
correctly, and explicit enough to support a replacement layout.

## Goal

Produce `docs/backend/x86_codegen_rebuild_plan.md` and
`docs/backend/x86_codegen_rebuild_handoff.md` so stage 3 can draft the
replacement subsystem against an explicit reviewed layout instead of inheriting
the stage-1 extraction set on trust.

## Core Rule

Do not draft replacement file contents or implementation edits during this
runbook. This stage owns review, diagnosis, extraction-set improvement
guidance, replacement layout, and the exact stage-3 handoff only.

## Read First

- `ideas/open/79_review_extracted_x86_codegen_subsystem_for_phoenix_rebuild.md`
- `ideas/closed/78_extract_x86_codegen_subsystem_to_markdown_for_phoenix_rebuild.md`
- `docs/backend/x86_codegen_legacy/`
- `docs/backend/x86_codegen_subsystem.md`

## Scope

- the full extraction set under `docs/backend/x86_codegen_legacy/`
- the real ownership, dispatch, helper, and hidden-dependency shape of the
  current `src/backend/mir/x86/codegen/` subsystem as reconstructed from that
  set
- extraction quality judgment: what is trustworthy as-is versus what still
  needs correction, expansion, compression, reclassification, or
  reorganization
- the replacement architecture layout stage 3 must draft, including the full
  `.cpp.md` / `.hpp.md` manifest it must produce
- the explicit handoff contract from stage 2 to stage 3

## Non-Goals

- writing replacement `.cpp.md` / `.hpp.md` bodies
- editing `src/backend/mir/x86/codegen/`
- converting markdown drafts into implementation
- deleting legacy code or choosing migration order beyond the owned handoff

## Working Model

- treat the stage-1 extraction set as evidence to review, not as an
  automatically trusted design
- reconstruct actual ownership and dependency direction before proposing
  replacement file boundaries
- call out false seams, stable seams, hidden state, and prepared-route
  parallel-stack behavior explicitly
- keep outputs compressed and decision-oriented rather than verbose artifact
  dumps
- preserve the motivating failure pressure from idea 75 as a design constraint
  when judging the replacement layout

## Execution Rules

- prefer `artifact audit -> subsystem reconstruction -> layout definition ->
  handoff validation`
- if the extraction set is weak in a specific area, record the required fix in
  the stage-2 output instead of silently trusting it
- name exact planned draft artifacts for stage 3; do not leave file layout
  implicit
- keep boundaries explicit when distinguishing canonical seams, compatibility
  seams, and overfit to reject
- keep the result reviewable by another agent without reopening the live
  source tree first

## Step 1: Audit Extraction Set Coverage And Compression

Goal: review the stage-1 artifact set for truthfulness, completeness, and
compression quality before using it as redesign input.

Primary targets:

- `docs/backend/x86_codegen_legacy/index.md`
- `docs/backend/x86_codegen_legacy/*.md`

Actions:

- verify the index still tells the truth about ownership buckets, dependency
  direction, prepared-route divergence, and proof surfaces
- identify per-file artifacts that need correction, expansion, compression,
  reclassification, or reorganization before later stages should trust them
- record where the extraction set still hides important contract or dependency
  facts

Completion check:

- the review names which stage-1 artifacts are trustworthy as-is and which
  must be treated as weak evidence or corrected inputs

## Step 2: Reconstruct Current Subsystem Seams And Failure Pressure

Goal: explain how the current subsystem actually routes ownership, dispatch,
shared helpers, and prepared-route bypasses.

Primary targets:

- `docs/backend/x86_codegen_rebuild_plan.md`
- `docs/backend/x86_codegen_legacy/`

Actions:

- reconstruct the real seam map across canonical lowering families, helper
  contracts, emitter boundaries, and the `prepared_*.cpp` stack
- identify false couplings, hidden dependencies, and responsibilities that are
  mixed together today
- judge which APIs and contracts are stable enough to preserve and which
  behaviors should be isolated as compatibility or rejected as overfit
- explicitly evaluate whether the current seam map explains the prepared-route
  runtime and call-lane pressure that surfaced in idea 75

Completion check:

- `docs/backend/x86_codegen_rebuild_plan.md` tells the truth about current
  subsystem behavior and its motivating failure pressure instead of merely
  restating the stage-1 file list

## Step 3: Define The Replacement Layout And Draft Manifest

Goal: convert the reviewed subsystem model into a concrete replacement
architecture layout that stage 3 can draft directly.

Primary targets:

- `docs/backend/x86_codegen_rebuild_plan.md`

Actions:

- define the replacement subsystem boundaries and file layout
- name every planned `.cpp.md`, every planned `.hpp.md`, and any directory or
  index markdown stage 3 must produce
- explain how the replacement layout separates stable seams from compatibility
  seams and prepared-route-specific pressure

Completion check:

- the rebuild plan contains a concrete mandatory draft manifest rather than a
  vague architecture sketch

## Step 4: Write Stage-3 Handoff And Validate Readiness

Goal: leave stage 3 with an explicit contract about what to consume, preserve,
correct first, and reject.

Primary targets:

- `docs/backend/x86_codegen_rebuild_handoff.md`
- `docs/backend/x86_codegen_rebuild_plan.md`

Actions:

- write the explicit stage-2-to-stage-3 handoff covering trusted extraction
  inputs, required corrections, mandatory draft artifacts, and route
  constraints
- ensure the handoff and rebuild plan agree on the replacement layout and the
  extraction-set weaknesses that remain relevant
- confirm the outputs are compressed enough to guide drafting without becoming
  another legacy dump

Completion check:

- both stage-2 outputs are present, aligned, and explicit enough for stage 3
  to execute without re-deriving layout or trust assumptions from scratch
