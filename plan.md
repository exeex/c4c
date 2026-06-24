# Textual Assembler Object Route Follow-Up Runbook

Status: Active
Source Idea: ideas/open/335_textual_assembler_object_route_followup.md
Activated after: ideas/closed/334_object_route_scan_and_default_readiness.md

## Purpose

Decide whether c4c still needs a scoped textual assembler route after direct
native object emission, and if so plan or implement only the smallest
c4c-emitted assembly subset that feeds the existing target fragment/object
model path.

## Goal

Keep the compiler primary object route direct (`backend machine model -> object
writer -> .o`) while deciding whether a separate `c4c-as` style path for
c4c-emitted `.s` adds enough value to pursue now.

## Core Rule

Do not make `--codegen obj` depend on printing and parsing assembly text. Any
textual assembler work must remain a separate compatibility/diagnostic route,
must reuse target encoders and the shared object model, and must not weaken
existing asm or direct-object tests.

## Read First

- `ideas/open/335_textual_assembler_object_route_followup.md`
- `ideas/open/329_native_object_emission_umbrella.md`
- `ideas/closed/334_object_route_scan_and_default_readiness.md`
- `todo.md`
- `src/apps/`
- `src/backend/`
- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/`

## Current Context

Direct native object output exists as an explicit dual-route option:

- `--codegen obj` is wired through the backend object facade and CLI.
- RV64 and AArch64 have target-owned object emission for the accepted smoke
  subset.
- Object-route scan/default-readiness closed with a 41/41 proof and a decision
  to keep object output explicit, not default.
- Existing asm-route tests remain meaningful and selected.

The textual assembler follow-up is therefore a decision point first, not an
automatic implementation mandate.

## Non-Goals

- Do not change `--codegen obj` semantics.
- Do not route direct compiler object output through textual assembly parsing.
- Do not build a full GNU-compatible assembler.
- Do not accept arbitrary external assembly syntax without scoped proof.
- Do not weaken existing asm-route, object-route, runtime, CLI, or diagnostic
  tests.
- Do not implement unrelated target relocation, encoder, parser, or backend
  semantic work solely to parse more assembly text.

## Execution Rules

- Start with inspection and decision recording before any implementation.
- If textual assembler work is not currently needed, record a reviewed
  no-work-needed decision and prepare the idea for lifecycle closure.
- If textual assembler work is needed, scope it to c4c-emitted assembly syntax
  and create or execute focused packets that reuse existing target encoders and
  object model APIs.
- Add tests beside direct object and asm-route tests, never by replacing them.
- Stop and split if the work expands into GNU assembler compatibility or broad
  target feature implementation.

## Step 1: Inspect Textual Assembler Need And Seams

Goal: decide whether this child should implement a scoped textual assembler
route now or close with a no-work-needed decision.

Actions:

- Inspect current direct object, asm output, CLI surfaces, and backend test
  helpers.
- Check whether a `c4c-as` binary, parser stub, assembler namespace, or
  existing reference integration point already exists in the repo.
- Identify the smallest c4c-emitted assembly subset that would be useful if an
  assembler route is still needed.
- Record in `todo.md`:
  - current direct object/asm coexistence evidence
  - whether textual assembler work is recommended now
  - if yes, the first implementation seam and proof command
  - if no, the closure rationale and remaining follow-up trigger

Completion check:

- `todo.md` records a defensible implement-vs-close recommendation for this
  child, with exact next-step ownership and proof or closure handoff.

## Step 2: Implement Or Close The Scoped Route

Goal: execute the Step 1 recommendation without expanding beyond the source
idea.

Actions if implementation is recommended:

- Add only the scoped parser/route needed for c4c-emitted assembly selected in
  Step 1.
- Reuse target encoders/object fragments and the shared object model.
- Add tests proving the textual route is separate from direct `--codegen obj`.
- Preserve existing direct object and asm-route tests.

Actions if no work is recommended:

- Keep implementation untouched.
- Record no-work-needed evidence in `todo.md`.
- Request plan-owner closure for idea 335.

Completion check:

- Either the scoped textual assembler proof is green, or `todo.md` records a
  reviewed no-work-needed decision and closure handoff.

## Step 3: Validate Follow-Up And Umbrella Handoff

Goal: finish the textual assembler follow-up and hand back to the native object
emission umbrella.

Actions:

- Run the selected proof for any implementation, or a docs/lifecycle proof for
  a no-work decision.
- Record remaining direct-object and asm-route boundaries.
- State whether umbrella idea 329 can proceed to final acceptance review.

Completion check:

- `todo.md` records final proof or no-work closure evidence, and identifies
  the next lifecycle action for umbrella idea 329.
