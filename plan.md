# AArch64 Scalar Cast Machine Printer Forms Runbook

Status: Active
Source Idea: ideas/open/300_aarch64_scalar_cast_machine_printer_forms.md
Activated: 2026-05-19
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Repair the focused scalar-cast machine-printer residual family split from the
post-299 backend-regex inventory.

## Goal

Make AArch64 scalar integer zero/sign extension forms reach assembly printing
with supported widths and structured register operands for the focused cases.

## Core Rule

Progress must repair scalar-cast lowering, operand publication, or structured
printer admission. Do not claim progress through expectation, allowlist,
unsupported, timeout, runner, or CTest-registration changes.

## Read First

- `ideas/open/300_aarch64_scalar_cast_machine_printer_forms.md`
- `ideas/open/295_backend_regex_failure_family_inventory.md`
- `ideas/closed/296_aarch64_fused_compare_branch_operand_forms.md`
- `ideas/closed/297_lir_to_bir_local_memory_admission.md`
- `ideas/closed/298_lir_to_bir_global_pointer_aggregate_projection.md`
- `ideas/closed/299_aarch64_scalar_immediate_materialize_or_encoding_fallback.md`
- `test_before.log`, as the accepted post-299 backend-regex proof: 352
  selected, 294 passed, and 58 failed

## Current Targets

- Focused c-testsuite cases: `00035`, `00105`, `00126`, `00134`, `00135`,
  `00151`, and `00208`.
- Scalar-cast machine-printer diagnostics involving unsupported `zero_extend`
  source/result widths and unstructured `sign_extend` register sources.
- AArch64 scalar integer cast lowering, machine operand publication,
  machine-node selection, and printer admission only as needed for this family.

## Non-Goals

- Do not edit expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration.
- Do not reopen closed owners 285 through 299 from failing counts alone.
- Do not fold symbol-store value printing, scalar mul/div/rem, stack-slot store
  source scratch, unsigned reduction, unselected machine-node, semantic
  `lir_to_bir`, runtime nonzero, runtime mismatch, or timeout residuals into
  this owner without generated-code or diagnostic evidence.
- Do not perform a broad AArch64 backend rewrite to solve one cast diagnostic.
- Do not use filename-specific or current-error-string-specific shortcuts.

## Working Model

- The umbrella inventory selected this family because all seven cases fail
  before runtime with scalar-cast machine-printer/frontend diagnostics.
- `zero_extend` failures are expected to involve unsupported source/result
  width publication or selection.
- `sign_extend` failures are expected to involve an extension source that has
  not been published as a structured register operand.
- A valid repair should make the printer see a supported extension form or
  force a prior lowering/materialization step that yields printable operands.

## Execution Rules

- Start with diagnostics or generated assembly for the focused seven cases.
- Change the narrowest semantic layer that owns the bad cast form.
- Keep each implementation packet tied to one cast sub-family when possible:
  zero-extension widths, sign-extension source structure, or shared operand
  publication.
- After each code slice, run fresh build proof and the supervisor-delegated
  focused c-testsuite subset before broader backend-regex proof.
- Record residual focused failures by cast form, not by filename alone.

## Ordered Steps

### Step 1: Reproduce and classify focused cast diagnostics

Goal: identify the concrete scalar-cast forms reaching the printer for the
seven focused cases.

Actions:

- Reproduce or inspect diagnostics for `00035`, `00105`, `00126`, `00134`,
  `00135`, `00151`, and `00208` under the AArch64 backend route.
- Classify each focused failure as zero-extension width/form,
  sign-extension source structure, or another scalar-cast form.
- Identify the lowering or machine operand publication surface that creates
  each bad printer input.
- Record the current focused classification in `todo.md`.

Completion check:

- `todo.md` lists each focused case with cast form, observed diagnostic or
  generated-code evidence, and the likely implementation surface.

### Step 2: Repair zero-extension supported-width publication

Goal: make zero-extension scalar casts reach the printer with supported
source/result widths or an earlier valid lowering fallback.

Actions:

- Inspect the zero-extension lowering and machine-node/operand publication
  path identified in Step 1.
- Add semantic handling for supported printable extension forms, or lower
  unsupported width combinations before printer admission.
- Keep the change general to source/result width semantics, not to filenames.
- Run build proof and the delegated focused subset.

Completion check:

- Focused zero-extension cases no longer fail from unsupported-width
  `zero_extend` printer diagnostics, and no non-cast residual bucket is pulled
  into this owner.

### Step 3: Repair sign-extension structured source publication

Goal: make sign-extension scalar casts reach the printer with a structured
register source or an earlier valid lowering fallback.

Actions:

- Inspect the sign-extension source operand path identified in Step 1.
- Ensure the cast source is published as a structured register operand before
  machine printing, or lower through a valid temporary/materialized form.
- Keep the repair within scalar cast operand semantics.
- Run build proof and the delegated focused subset.

Completion check:

- Focused sign-extension cases no longer fail from unstructured-source
  `sign_extend` printer diagnostics, and the fix does not change unrelated
  runtime or symbol-store behavior.

### Step 4: Prove focused closure and report residuals

Goal: establish whether the scalar-cast owner is complete and leave unrelated
residual buckets parked under the umbrella.

Actions:

- Run the supervisor-selected focused proof for all seven target cases after
  the implementation slices are complete.
- Run the supervisor-selected broader backend-regex proof if the focused proof
  is green or materially improved.
- Report remaining failures by scalar-cast form and separate all unrelated
  residual buckets.
- Do not close this owner unless the old scalar-cast printer diagnostics are
  gone from the focused scope.

Completion check:

- The focused scalar-cast machine-printer diagnostics are gone, proof logs are
  recorded, and any remaining failures are either outside this owner or
  explicitly named as scalar-cast residuals.
