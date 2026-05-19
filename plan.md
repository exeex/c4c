# AArch64 Sign-Extension Assembler Legality Runbook

Status: Active
Source Idea: ideas/open/303_aarch64_sign_extension_assembler_legality.md
Switched From: ideas/open/302_aarch64_scalar_machine_node_operand_forms.md for the split sign-extension legality owner

## Purpose

Repair the AArch64 backend route that emits illegal sign-extension assembly
spelling after scalar operand-form lowering has moved `00205` farther through
backend validation.

## Goal

Make generated sign-extension instructions obey AArch64 register-width legality
without testcase-specific string handling.

## Core Rule

Treat this as a semantic AArch64 sign-extension width/spelling repair. Do not
change expectations, allowlists, unsupported classifications, timeout policy,
runner behavior, or CTest registration to claim progress.

## Read First

- Source idea: `ideas/open/303_aarch64_sign_extension_assembler_legality.md`
- Split source: `ideas/open/302_aarch64_scalar_machine_node_operand_forms.md`
- Current exposed failure: `00205` backend assembler validation on
  `sxtw w9, w13`
- Do not claim pass-count progress from the dirty idea 302 Step 3 code while
  this split owner is active.

## Current Targets

- Illegal AArch64 sign-extension spelling with a W destination register,
  currently observed as `sxtw w9, w13` in generated `00205` assembly.
- The backend selection or printer path that chooses sign-extension opcode and
  destination/source register widths for compare preparation or fused
  compare-branch inputs.

## Non-Goals

- Do not continue scalar arithmetic/reduction operand-form work from idea 302
  under this owner.
- Do not perform a broad fused compare-branch rewrite unrelated to
  sign-extension legality.
- Do not reopen closed owners from counts alone.
- Do not match only c-testsuite filenames, generated temporary names, line
  numbers, or exact diagnostic strings.

## Working Model

- The failure is generated-assembly legality, not the old scalar
  `logical_shift_right` unsigned-reduction printer diagnostic.
- `sxtw` sign-extends a 32-bit source into a 64-bit destination, so the backend
  must select a legal destination width, a legal 32-bit extension spelling, or
  no extension when the value is already in the required width.
- A valid slice may start from `00205`, but acceptance requires a general width
  rule in the sign-extension route.

## Execution Rules

- Start by tracing where the illegal sign-extension operation is selected or
  printed.
- Prefer a semantic width/opcode/register-class rule over printer-only string
  substitution.
- Keep implementation changes scoped to sign-extension legality unless
  generated-code evidence proves a shared helper boundary.
- Record fresh build proof and the supervisor-selected focused backend subset
  in `todo.md`.
- Report any later residual `00205` failure separately from this owner.

## Steps

### Step 1: Inspect Sign-Extension Route

Goal: identify the backend route that emits `sxtw` with a W destination.

Primary target: focused backend c-testsuite case `00205`

Actions:

- Inspect the generated assembly and backend route for the failing
  sign-extension operation.
- Trace the selected compare, branch, or extension node to the printer or
  instruction-selection helper that chooses opcode and register widths.
- Identify whether the repair belongs in selection, register-width
  materialization, or printer admission.

Completion check:

- `todo.md` records the active illegal sign-extension spelling, the emitting
  implementation surface, and the semantic width rule needed for repair.

### Step 2: Repair Sign-Extension Width Spelling

Goal: emit legal AArch64 sign-extension assembly for the affected route.

Primary target: sign-extension opcode/register-width selection

Actions:

- Repair the semantic path that allows `sxtw` to be emitted with a W
  destination.
- Keep the implementation general for sign-extension width legality.
- Build and prove the focused subset selected by the supervisor.

Completion check:

- The focused proof no longer fails because generated assembly contains
  `sxtw` with a W destination, and no expectation or runner contract changed.

### Step 3: Residual Classification And Handoff

Goal: separate this owner's completion from any later backend residual exposed
by the same testcase.

Primary target: focused proof results after the sign-extension repair

Actions:

- Re-run the supervisor-selected focused proof after the implementation slice.
- If `00205` fails later for a different reason, record that as a separate
  residual rather than expanding this owner silently.
- Ask the supervisor whether to reactivate idea 302, continue a new split
  owner, or return to the umbrella inventory.

Completion check:

- `todo.md` records proof for the sign-extension legality fix and names any
  remaining residual bucket without claiming unrelated pass-count progress.
