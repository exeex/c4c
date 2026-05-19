# AArch64 Scalar Immediate Materialize Or Encoding Fallback Runbook

Status: Active
Source Idea: ideas/open/299_aarch64_scalar_immediate_materialize_or_encoding_fallback.md
Activated: 2026-05-19

## Purpose

Repair the scalar AArch64 machine-printing path that receives non-encodable
add/sub/bitwise immediates instead of a materialized register or structured
printable fallback.

## Goal

Make the focused scalar immediate residuals compile past the old
machine-printer diagnostic without weakening test contracts or folding
unrelated buckets into this owner.

## Core Rule

Progress must come from semantic AArch64 lowering, operand normalization,
materialization, or encoding fallback for scalar immediates, not from
testcase-shaped matching or expectation changes.

## Read First

- `ideas/open/299_aarch64_scalar_immediate_materialize_or_encoding_fallback.md`
- `ideas/open/295_backend_regex_failure_family_inventory.md`
- `ideas/closed/296_aarch64_fused_compare_branch_operand_forms.md`
- `ideas/closed/297_lir_to_bir_local_memory_admission.md`
- `ideas/closed/298_lir_to_bir_global_pointer_aggregate_projection.md`
- `todo.md`

## Current Targets

- Focused cases: `00031`, `00104`, `00143`, `00207`, `00213`, `00214`,
  `00215`, and `00218`.
- Scalar add/subtract and bitwise immediate operands that are outside the
  valid AArch64 instruction-immediate encoding range.
- Generated assembly or diagnostics proving constants are materialized or
  selected through a valid printable fallback.

## Non-Goals

- Do not reopen closed owners 285 through 298 from failing counts alone.
- Do not repair scalar casts, mul/div/rem, call-boundary moves, memory
  store/symbol printing, runtime nonzero/mismatch cases, or timeout `00220`
  under this plan unless new proof shows the same scalar immediate owner.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration.
- Do not add filename-specific or instruction-string-specific shortcuts.
- Do not print invalid AArch64 immediates merely to get past the diagnostic.

## Working Model

- The post-298 inventory selected this owner because the focused cases share a
  machine-printer symptom: selected scalar add/xor/and or nearby
  add/sub/bitwise forms carry constants as plain immediates that the printer
  cannot encode.
- Valid repair routes include selecting an encodable immediate form when legal,
  materializing non-encodable constants into registers, or routing through an
  existing structured fallback that prints valid AArch64 assembly.
- Runtime nonzero and mismatch buckets need generated assembly or narrower
  probes before they can become focused owners.

## Execution Rules

- Keep routine progress and proof notes in `todo.md`.
- Prefer small implementation packets with fresh build proof plus the focused
  c-testsuite subset.
- Escalate to broader backend-regex validation when the focused diagnostic
  bucket changes or when the touched path affects shared scalar lowering.
- Preserve residual bucket classification when reporting proof.

## Ordered Steps

### Step 1: Reproduce and localize the focused printer failures

Goal: confirm the exact current diagnostic and selected operand shapes for the
focused scalar immediate cases.

Actions:

- Run a narrow proof over `00031`, `00104`, `00143`, `00207`, `00213`,
  `00214`, `00215`, and `00218` after a current build.
- Capture enough generated assembly, diagnostics, or backend traces to show
  which scalar add/sub/bitwise forms carry non-encodable immediates.
- Separate true scalar immediate encoding failures from any case that has
  moved to a different failure mode.

Completion check:

- `todo.md` records the focused cases, current failure modes, and the first
  implementation surface to inspect.

### Step 2: Repair scalar immediate lowering or fallback

Goal: ensure non-encodable scalar add/sub/bitwise constants are represented in
a printable AArch64 form before machine printing.

Actions:

- Inspect the selector, operand normalization, materialization, and printer
  path that handles scalar add/sub/bitwise immediates.
- Preserve existing valid immediate forms when the constant is encodable.
- For non-encodable constants, materialize to a register or route through an
  existing structured fallback before printing.
- Avoid broad AArch64 rewrites outside scalar immediate handling.

Completion check:

- Representative focused cases reach valid generated assembly or a later
  non-printer failure without the old non-encodable-immediate diagnostic.

### Step 3: Prove focused and residual behavior

Goal: validate the repair without hiding unrelated backend-regex residuals.

Actions:

- Run fresh build proof.
- Run the focused c-testsuite backend subset for the eight target cases.
- If the focused bucket improves, run the supervisor-selected broader backend
  regex proof and classify remaining failures by bucket.
- Record proof commands, pass/fail counts, and residual bucket movement in
  `todo.md`.

Completion check:

- Focused proof is green or any remaining failures are classified outside the
  original scalar immediate printer diagnostic, and broader proof has enough
  classification for the supervisor to decide acceptance or the next split.
