# LIR Intrinsic And Inline-Assembly Binding Evidence Runbook

Status: Exhausted - Pending Plan-Owner Decision
Source Idea: ideas/open/849_lir_intrinsic_binding_evidence.md
Activated after: ideas/closed/848_lir_global_policy_identity_evidence.md

## Purpose

Produce the documentation evidence required to classify intrinsic operand
bindings and `LirInlineAsmValueBinding` routes before any 796, 846, 734, or
797 follow-up can claim coverage.

## Goal

Trace intrinsic and inline-assembly binding producers through verifier and
lowering consumers, while proving inline-assembly templates and constraints
remain opaque outward payload.

## Core Rule

This is research and architecture documentation only. Do not edit
implementation files, tests, expectations, unsupported markers, allowlists,
runtime behavior, or lifecycle history beyond this active plan.

## Read First

- `ideas/open/849_lir_intrinsic_binding_evidence.md`
- `docs/lir_string_authority_remaining_routes/handoff_to_813.md`
- `docs/lir_string_semantic_authority_completion/input_validation.md`
- `ideas/open/796_lir_instruction_terminator_residual_authority_handoff.md`
- `ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/open/797_lir_to_new_bir_final_coverage_convergence.md`

## Current Scope

- `LirInlineAsmValueBinding` producers, verifier boundaries, and lowering
  consumers.
- Intrinsic forms and their binding representations.
- Positive binding evidence and applicable malformed, foreign, or mismatch
  expectations.
- Return/dependency relations to 796, 846, 734, and 797.

## Non-Goals

- No implementation changes.
- No combined residual instruction sweep.
- No parsing inline-assembly templates or constraints for binding, type, value,
  ABI, or dispatch facts.
- No reopening accepted 796 cast-result or pointer-subtraction work.
- No 734 receiver or 797 convergence claim before an exact accepted handoff.

## Working Model

849 is the ordered evidence successor after 848. It may classify a future
repair route to 796 or 846, but it must not implement that repair or return
directly to 734 without an accepted producer/verifier handoff.

## Execution Rules

- Create exactly the required documentation directory and files:
  `docs/lir_intrinsic_binding_evidence/index.md` and
  `docs/lir_intrinsic_binding_evidence/01_intrinsic_inline_asm_binding_route.md`.
- The numbered answer file must contain the producer-to-verifier-to-lowerer
  inventory; the index only summarizes and links it.
- Report unknown coverage as missing evidence.
- Keep templates and constraints explicitly opaque.
- Keep lifecycle proof to documentation/file checks unless the source is
  revised by a later supervisor decision.

## Ordered Steps

### Step 1 - Inventory Binding Producers And Intrinsic Forms

Status: Complete

Goal: identify every discovered inline-assembly binding producer and intrinsic
form relevant to the routing key.

Actions:
- Search for `LirInlineAsmValueBinding` construction and consumption.
- Search intrinsic lowering and verifier paths that carry native binding
  facts.
- Classify each discovered form by binding representation.

Completion check:
- The numbered answer contains a complete discovered inventory with concrete
  source locations and explicit missing-evidence notes.

### Step 2 - Trace Verifier And Lowerer Boundaries

Status: Complete

Goal: map each binding representation to verifier and lowering consumers.

Actions:
- Record verifier checks for malformed, foreign, or mismatch binding where
  present.
- Record lowerer/importer consumers and the facts they rely on.
- State when a form has no native binding contract.

Completion check:
- The numbered answer distinguishes proven binding routes from missing or
  unproved routes.

### Step 3 - Prove Opacity And Return Relations

Status: Complete

Goal: define the policy boundary and downstream ownership.

Actions:
- Show that inline-assembly templates and constraints remain opaque and are not
  parsed for binding, type, value, ABI, or dispatch facts.
- Assign any future narrow repair to 796 or 846 only with a singular family and
  native fact.
- State that 734 and 797 remain downstream.

Completion check:
- The numbered answer and index agree on opacity, repair ownership, and
  downstream return conditions.

### Step 4 - Validate Documentation Shape

Status: Complete

Goal: prove the deliverable is complete and bounded.

Actions:
- Check that `docs/lir_intrinsic_binding_evidence/` contains exactly
  `index.md` and `01_intrinsic_inline_asm_binding_route.md`.
- Run `git diff --check`.
- Run any lightweight text/link checks the repository already provides if a
  matching docs command exists.

Completion check:
- Required files exist with the required shape, no extra files are present in
  the directory, `git diff --check` passes, and no implementation files are
  modified.

Result:
- Created `docs/lir_intrinsic_binding_evidence/index.md`.
- Created
  `docs/lir_intrinsic_binding_evidence/01_intrinsic_inline_asm_binding_route.md`.
- Recorded that structured inline-asm value bindings have verifier and selected
  Raw-BIR routes, templates/constraints remain opaque, and no direct 734
  receiver handoff is authorized without a later singular 796 or 846 handoff.
- Proof passed with exact required-file listing and `git diff --check`.
