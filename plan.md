# AArch64 Machine Status And Printer Validation Probe Runbook

Status: Active
Source Idea: ideas/open/77_aarch64_machine_status_printer_validation_probe.md

## Purpose

Compare AArch64 machine status derivation with machine-printer validation for
the same record families before deciding whether repeated semantic validation
should fold into an AArch64-local helper.

## Goal

Classify status and printer validation overlap by record family, then choose a
bounded no-code conclusion or implementation route.

## Core Rule

Do not move final assembly spelling validation out of the printer, and do not
add a helper until the probe proves repeated semantic validation for the same
record family.

## Read First

- `ideas/open/77_aarch64_machine_status_printer_validation_probe.md`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- Existing focused machine-status and printer-output tests for AArch64
  record families touched by the probe.

## Current Scope

- Compare machine status derivation helpers against printer validation checks
  for matching AArch64 record families.
- Distinguish semantic validation from final mnemonic, register, relocation,
  address, and assembly text spelling validation.
- If repeated semantic validation exists, propose or implement one
  AArch64-local helper boundary beside the existing AArch64 owners.

## Non-Goals

- Do not move machine record schemas or printer spelling into shared code.
- Do not treat all status and printer validation as duplication without a
  record-family comparison.
- Do not bundle broad instruction schema cleanup or printer formatting
  rewrites into this probe.
- Do not combine this probe with scalar register helper fold-back unless the
  evidence proves the same helper boundary should own both.
- Do not weaken tests or downgrade unsupported expectations as proof.

## Working Model

- Machine record construction and final printer spelling are target-local by
  default.
- Semantic validation may be duplicated only when status derivation and printer
  validation check the same record-family facts for the same reason.
- Final assembly text validation belongs in the printer even if nearby status
  derivation performs related semantic checks.
- Implementation is optional and follows only if the evidence names a concrete
  repeated semantic validation boundary.

## Execution Rules

- Record probe findings in `todo.md` before code changes.
- Keep the source idea stable unless durable intent changes.
- Keep packets narrow: identify record families first, classify checks second,
  implement only if required.
- Every code-changing step needs fresh build proof plus focused machine-status
  and printer-output tests for the affected record families.
- Use canonical regression logs before accepting any code slice.

## Step 1: Inventory Status And Printer Record Families

Goal: identify the AArch64 record families that appear in both machine status
derivation and machine-printer validation.

Primary targets:
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

Actions:
- List the status derivation helpers and the record families they inspect.
- List the printer validation checks and the record families they inspect.
- Pair only record families that appear in both owners.
- Record the inventory in `todo.md` with exact function/helper names.

Completion check:
- `todo.md` contains the paired record-family inventory and identifies any
  printer-only or status-only families.
- No implementation files have changed for this evidence-only step.

## Step 2: Classify Validation Overlap

Goal: decide which paired checks are repeated semantic validation and which are
final assembly text validation that should remain in the printer.

Actions:
- For each paired record family from Step 1, compare the facts checked by
  status derivation and printer validation.
- Classify each overlap as semantic validation, final mnemonic/register/
  relocation/address/text validation, or non-overlap.
- Name any exact duplicated condition and the reason each owner currently
  checks it.
- Record the classification in `todo.md`.

Completion check:
- `todo.md` clearly separates repeated semantic validation from printer-owned
  final spelling validation for each paired record family.
- Reviewer reject signals from the source idea are checkable from the recorded
  evidence.

## Step 3: Decide AArch64-Local Helper Boundary

Goal: choose whether repeated semantic validation should fold into one
AArch64-local helper or remain separately target-local.

Actions:
- Compare the Step 2 classifications across record families.
- If repeated semantic validation exists, name the proposed helper boundary,
  owner file, inputs, consumers, and responsibilities.
- If the overlap is only final spelling validation or intentionally separate
  status/printer behavior, record the no-code conclusion.
- Keep final assembly text output and spelling checks in
  `machine_printer.cpp`.

Completion check:
- `todo.md` or a plan checkpoint states one concrete route: no-code evidence
  conclusion, implement an AArch64-local semantic validation helper, or create
  a separate follow-up idea for a different initiative.
- The route does not expand beyond the record families proven by the probe.

## Step 4: Implement Helper If Required

Goal: remove only the proven repeated semantic validation by introducing or
extending an AArch64-local validation helper.

Primary targets:
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- Any new AArch64-local helper beside these owners.

Actions:
- Add the helper boundary named in Step 3, or extend the chosen existing local
  helper if one already owns the semantic check.
- Migrate only the status and printer checks proven to duplicate semantic
  validation.
- Leave final mnemonic spelling, register spelling, relocation/address
  spelling, and assembly text output in the printer.
- Avoid broad instruction schema cleanup, printer formatting rewrites, or
  scalar register helper fold-back unless Step 3 proved the same boundary.

Completion check:
- Focused tests cover machine status and printer output for the affected
  record families.
- Build proof is fresh.
- Regression guard accepts matching before/after logs for the chosen scope.

## Step 5: Acceptance Review And Closure Decision

Goal: decide whether idea 77 is complete or whether a separate follow-up idea
is required.

Actions:
- Compare the final evidence and any implementation against the source idea.
- Reject helper renames that leave duplicated validation in both owners,
  printer-spelling migrations without semantic proof, expectation downgrades,
  or bundled broad rewrites.
- Run or consume broader regression guard coverage appropriate to any touched
  implementation surface.
- If a separate initiative remains, create it under `ideas/open/` instead of
  expanding this plan silently.

Completion check:
- A reviewer can see which record families were compared, which checks were
  semantic versus final text validation, and whether implementation was
  justified.
- The active lifecycle state is ready for close, deactivation, or plan rewrite.
