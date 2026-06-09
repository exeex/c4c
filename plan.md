# AArch64 Dispatch Family Post-Contract Layout Audit

Status: Active
Source Idea: ideas/open/130_aarch64_dispatch_family_post_contract_layout_audit.md

## Purpose

Re-audit the AArch64 `dispatch*` codegen family after the producer,
edge-publication, current-block, calls, memory, wide-value, and i128-shift
contract work has closed.

## Goal

Classify every remaining dispatch-family source and header by durable owner
boundary, then produce focused follow-up ideas only for concrete fold-back or
shared-contract gaps.

## Core Rule

This runbook is analysis-only. Do not edit implementation code, move
dispatch-family code, weaken tests, or claim cleanup progress through file
count reduction alone.

## Read First

- `ideas/open/130_aarch64_dispatch_family_post_contract_layout_audit.md`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- matching `src/backend/mir/aarch64/codegen/dispatch*.hpp` files
- `ref/claudes-c-compiler/src/backend/arm/codegen`

## Scope

Classify each dispatch-family `.cpp` and `.hpp` file as one or more of:

- `keep-public-hook`
- `fold-back`
- `shared-contract-gap`
- `target-local-emission`
- `local-organization-only`

The closure note must include:

- an ownership table for every `dispatch*.cpp` and `dispatch*.hpp` file
- a call-site map for public dispatch-family declarations
- explicit no-new-idea notes for retained public hooks
- bounded follow-up ideas for concrete fold-back or shared-contract gaps
- a note on whether physical file count can shrink without behavior changes

## Non-Goals

- Do not move implementation code during this audit.
- Do not merge public hook surfaces as part of this runbook.
- Do not introduce or remove build targets.
- Do not reclassify expectations, weaken backend tests, or mark supported
  behavior unsupported.
- Do not move AArch64 register hazard policy, instruction spelling, or final
  emission into shared BIR/prealloc code.

## Working Model

Treat a separate dispatch translation unit as justified only when it owns a
stable public hook, a useful query boundary, or target-local emission grouping
that is clearer outside `dispatch.cpp`. Treat thin target-local wrappers,
temporary helper splits, and rediscovered shared facts as candidates for
follow-up ideas, not immediate edits.

## Execution Rules

- Preserve source intent in the source idea; record execution progress in
  `todo.md`.
- Use structured symbol and call-site inspection where available before
  drawing ownership conclusions.
- Compare current AArch64 layout to the reference ARM layout as evidence, not
  as a mandate to copy the reference.
- Any follow-up idea must name bounded files, proof route, and reject signals.
- If a file should remain separate, write a concrete no-new-idea rationale.
- If a classification depends on a public function, include its callers in the
  call-site map.

## Steps

### Step 1: Build Dispatch Family Inventory

Goal: Establish the complete audited file set and public declaration surface.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch*.cpp`
- `src/backend/mir/aarch64/codegen/dispatch*.hpp`
- relevant build metadata that enumerates these files

Actions:

- List every dispatch-family `.cpp` and `.hpp` file in the AArch64 codegen
  directory.
- Record public declarations in each matching header.
- Map each dispatch-family implementation file to its declared public
  functions and file-local helper responsibilities.
- Identify build metadata entries for the dispatch-family files.

Completion check:

- `todo.md` contains the complete inventory, declaration surface, and build
  metadata touchpoints needed for later classification.

### Step 2: Map Public Call Sites

Goal: Determine which dispatch-family public declarations are externally used
hooks and which are private organization artifacts.

Primary targets:

- all callers of public declarations from `dispatch*.hpp`
- `dispatch_edge_copies.cpp` entry points retained by idea 81
- `dispatch_producers.cpp` consumers after idea 116

Actions:

- For each public declaration, locate direct call sites and include call-site
  files in the map.
- Separate external orchestration hooks from functions called only by nearby
  dispatch-family code.
- Pay special attention to edge-copy hooks, producer wrappers, publication
  helpers, value-materialization helpers, and lookup queries.

Completion check:

- `todo.md` contains a call-site map sufficient to justify
  `keep-public-hook`, `fold-back`, or `local-organization-only`
  classification for each public declaration.

### Step 3: Classify Ownership And Contract Boundaries

Goal: Assign durable responsibility labels to every dispatch-family file.

Primary targets:

- each audited `dispatch*.cpp` and `dispatch*.hpp`
- shared prepared facts introduced or closed by prior contract work

Actions:

- Classify each file as one or more of the source-idea categories.
- Check for remaining target-neutral producer, publication, edge-copy,
  current-block, or select-chain fact rediscovery in AArch64 codegen.
- Distinguish target-local emission glue from reusable shared policy.
- Decide whether `dispatch_lookup.cpp` is a useful query boundary or a thin
  helper split.
- Decide whether `dispatch_publication.cpp` and
  `dispatch_value_materialization.cpp` still justify separate translation
  units.

Completion check:

- `todo.md` contains an ownership table covering every audited file and a
  concrete rationale for each classification.

### Step 4: Draft Follow-Up Ideas Or No-New-Idea Notes

Goal: Convert concrete fold-back and shared-contract findings into bounded
future work, and explicitly retain justified public hooks.

Primary targets:

- candidate `fold-back` classifications
- candidate `shared-contract-gap` classifications
- files classified as `keep-public-hook`

Actions:

- For each concrete fold-back candidate, draft a follow-up idea with files,
  expected behavior preservation, proof route, and reject signals.
- For each shared-contract gap, draft a follow-up idea that names the missing
  shared fact and rejects target-local rediscovery.
- For every retained public hook, write an explicit no-new-idea rationale.
- Avoid vague cleanup ideas or line-count/file-count-only justifications.

Completion check:

- Each actionable finding has either a bounded follow-up idea draft or an
  explicit no-new-idea note.

### Step 5: Finalize Audit Closure Material

Goal: Leave the source idea ready for closure review without implementation
edits.

Primary targets:

- `todo.md` audit notes
- any new `ideas/open/*.md` follow-up files required by the audit findings
- the active source idea closure note, only if lifecycle rules require closure

Actions:

- Assemble the dispatch-family ownership table, call-site map, no-new-idea
  notes, follow-up idea list, and file-count-shrink conclusion.
- If follow-up ideas are needed, ensure each has bounded files, proof route,
  and reviewer reject signals.
- If no follow-up is warranted, write the concrete explanation for retaining
  the current dispatch-family split.
- Do not close the source idea unless the audit criteria are satisfied and
  lifecycle closure is explicitly performed.

Completion check:

- The supervisor can review the completed audit artifacts and decide whether
  to close the source idea or continue with follow-up lifecycle work.
