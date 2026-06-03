# Prealloc Stack Layout Slice Family Fact Contract Runbook

Status: Active
Source Idea: ideas/open/106_prealloc_stack_layout_slice_family_fact_contract.md

## Purpose

Replace or explicitly justify stack-layout slot/slice-family reconstruction from
names and BIR instruction scans with a structured placement-analysis contract.

## Goal

Classify stack-layout hint paths that infer slot families, aggregate slice
exposure, address publication, pointer roots, or coalescing facts, then either
retain them as named prealloc placement analysis or move target-neutral facts to
explicit BIR/prepared authority.

## Core Rule

Do not move physical stack layout into BIR. Frame-slot IDs, offsets, frame size,
alignment, and home-slot placement stay in prealloc. The cleanup target is the
narrow family/publication fact boundary: name-based or instruction-scan
reconstruction must be classified, narrowed, or replaced with structured facts.

## Read First

- `ideas/open/106_prealloc_stack_layout_slice_family_fact_contract.md`
- `src/backend/prealloc/stack_layout/*.cpp`
- Prepared memory access and address-publication surfaces
- Existing backend tests for stack layout, aggregate slices, address
  publication, alloca coalescing, and copy coalescing

## Current Targets

- Stack-layout hint paths that infer slot families from names.
- BIR instruction scans that infer aggregate slice exposure or pointer roots.
- Address-publication hints used by stack-layout analysis.
- Alloca and copy coalescing routes affected by slice-family reconstruction.

## Non-Goals

- Do not move frame offsets, stack object ordering, frame size, or home-slot
  placement into BIR.
- Do not rebuild alloca or copy coalescing broadly.
- Do not change unrelated prepared memory access or pointer-carrier contracts.
- Do not treat every BIR scan in prealloc as duplication without naming the
  reconstructed fact.

## Working Model

Stack object creation, frame-slot assignment, storage plans, and decoded homes
are intentional prealloc placement/storage authority. The ambiguity is narrower:
some stack-layout hints infer family/publication facts from names or instruction
shape. Those scans may be legitimate placement-only analysis, but they need a
clear contract so target-neutral slice-family or publication facts are not
silently reconstructed in prealloc.

## Execution Rules

- Keep inventory and classification notes in `todo.md`.
- Name the fact being reconstructed before deciding whether a scan is
  duplication.
- Prefer structured BIR/prepared facts for target-neutral family/publication
  relations.
- Retain prealloc placement analysis only when it is explicitly non-semantic
  and tied to home-slot/coalescing decisions.
- For code-changing steps, run `cmake --build --preset default` before handing
  the packet back.
- Escalate validation if shared stack-layout, prepared memory, or coalescing
  interfaces change.

## Step 1: Inventory Stack Layout Reconstruction Routes

Goal: trace stack-layout hint paths that infer slot families, aggregate slice
exposure, address publication, pointer roots, or coalescing facts from names or
BIR instruction scans.

Primary targets:

- `src/backend/prealloc/stack_layout/*.cpp`
- Prepared memory/address publication helpers
- Existing backend stack-layout and coalescing tests

Actions:

- Trace name-based family inference routes.
- Trace BIR instruction scans used for aggregate slice exposure.
- Trace address-publication and pointer-root inference used by stack-layout
  hints.
- Trace alloca and copy coalescing paths that depend on reconstructed family
  facts.
- Separate physical placement decisions from target-neutral family/publication
  facts.
- Record current behavior, likely owner, and proof gaps in `todo.md`.

Completion check:

- `todo.md` lists each owned reconstruction route and its current authority.
- Analysis proof confirms no implementation diff under `src/backend/bir` or
  `src/backend/prealloc`.

## Step 2: Decide The Slice-Family Fact Contract

Goal: classify each reconstruction route as retained placement analysis or
replace it with explicit BIR/prepared authority.

Actions:

- Compare Step 1 findings against the source idea acceptance criteria.
- Decide which name-based family routes must be removed, narrowed, or retained
  as documented compatibility.
- Decide which BIR scans are pure prealloc placement analysis and which expose
  missing target-neutral facts.
- Identify proof targets for address publication and coalescing/slice-family
  behavior.
- Reject routes that only rename helpers while preserving semantic
  reconstruction.

Completion check:

- `todo.md` records the contract decision, implementation targets, retained
  placement-only paths if any, and proof targets.
- Analysis proof confirms no implementation diff unless this packet
  intentionally begins implementation.

## Step 3: Implement Or Narrow The Contract

Goal: make stack-layout slice-family and publication consumers use explicit
authority or named placement-only analysis according to the Step 2 contract.

Actions:

- Remove or narrow name-based family inference where it acts as semantic
  authority.
- Replace target-neutral family/publication reconstruction with explicit
  BIR/prepared facts where required.
- Document retained compatibility paths only when they are narrow and
  non-primary.
- Keep frame-slot IDs, offsets, frame size, alignment, and home-slot placement
  in prealloc.
- Avoid unrelated prepared memory, pointer-carrier, alloca, or copy-coalescing
  rewrites.

Completion check:

- The default build passes.
- The diff does not add named-case slot matching or expectation rewrites.
- Each changed route is reviewable as either structured fact consumption or
  named prealloc placement-only analysis.

## Step 4: Add Focused Stack Layout Proof

Goal: prove address-publication behavior and coalescing/slice-family behavior
against the chosen contract.

Actions:

- Add or strengthen tests for an address-publication case affected by the old
  reconstruction path.
- Add or strengthen tests for a coalescing or aggregate slice-family case.
- Prefer prepared stack-layout, decoded-home, or contract assertions over
  fragile assembly-only symptoms when possible.
- Include fail-closed proof if a former semantic reconstruction path is
  removed.

Completion check:

- The default build passes.
- Focused backend stack-layout/coalescing tests pass.
- Tests cover both required acceptance families and do not weaken existing
  contracts.

## Step 5: Final Validation And Close Readiness

Goal: prove the completed route and prepare the source idea for closure review.

Actions:

- Run the default build and the relevant backend stack-layout/coalescing subset.
- Include broader `^backend_` validation if shared stack-layout, prepared
  memory, or coalescing interfaces changed.
- Update `todo.md` with final proof, retained compatibility or placement-only
  details, and close-readiness notes.

Completion check:

- Final delegated proof passes.
- `todo.md` shows address-publication and coalescing/slice-family coverage.
- The source idea acceptance criteria are satisfied without moving physical
  stack layout into BIR or rewriting unrelated contracts.
