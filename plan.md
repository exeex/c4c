# Inline Assembly Memory And Address Constraints Runbook

Status: Active
Source Idea: ideas/open/242_inline_asm_memory_address_constraints.md

## Purpose

Activate the memory/address constraint route from the inline-asm machine-node
work without adding allocator, spill, or scratch-register policy in selection
or printing.

## Goal

Support inline-asm memory and address constraints only when prepared homes carry
complete target-valid authority; fail closed otherwise.

## Core Rule

Do not infer memory or address operands from rendered assembly text, textual
operand spelling, testcase names, or local printer/dispatch allocation choices.
Selection may accept a memory/address operand only from structured prepared-home
facts that are complete for the AArch64 target.

## Read First

- `ideas/open/242_inline_asm_memory_address_constraints.md`
- Inline-asm prepared-home and operand classification surfaces in the current
  backend route.
- Existing AArch64 inline-asm tests for operands, names, immediates,
  modifiers, side effects, outputs, clobbers, and numeric tied homes.

## Current Scope

- Define the prepared-home authority needed for inline-asm memory operands.
- Define the prepared-home authority needed for inline-asm address operands.
- Preserve structured BIR/prepared facts so AArch64 selection can distinguish
  supported target-valid homes from unsupported or incomplete forms.
- Select and print memory/address operands only when those homes are complete.
- Diagnose unsupported constraints, missing homes, and target-invalid address
  forms explicitly.
- Prove a supported representative when the current prepared-home model can
  express one; otherwise record a precise blocker in `todo.md`.

## Non-Goals

- Do not build an inline-asm-specific register allocator, spill planner, or
  scratch-register convention.
- Do not parse memory/address semantics from final rendered AArch64 assembly.
- Do not weaken unsupported-path tests to claim support.
- Do not solve clobber ingress in this route.
- Do not solve alias-aware tied-home coallocation in this route.

## Working Model

- Memory and address constraints are authority-sensitive operands.
- Prepared-home data is the trust boundary between frontend/BIR facts and
  AArch64 selection.
- Missing, partial, allocator-dependent, or target-invalid homes remain
  unsupported even when a narrow testcase could be printed textually.
- Diagnostics are part of the contract: unsupported forms should fail closed
  with reasons that distinguish missing homes from invalid homes.

## Execution Rules

- Keep each implementation step semantic and structured; reject fixture-shaped
  matching.
- Prefer extending existing prepared-home/inline-asm data paths over adding a
  parallel memory/address-only protocol.
- Preserve existing supported inline-asm behavior while adding fail-closed
  coverage near the new route.
- Record blockers in `todo.md` when the prepared-home model cannot express a
  supported representative yet; do not mask that gap with expectation rewrites.
- For code-changing steps, run at least a fresh build or compile proof plus the
  supervisor-selected narrow inline-asm test subset. Escalate to broader
  regression guard when the touched surface crosses backend contracts.

## Ordered Steps

### Step 1: Inspect Prepared-Home And Constraint Surfaces

Goal: identify the existing structured facts available for inline-asm memory
and address operands.

Primary targets:

- Inline-asm BIR representation and prepared-home records.
- AArch64 inline-asm selection and printing paths.
- Existing backend tests for supported and unsupported inline-asm constraints.

Actions:

- Trace how existing inline-asm operands carry prepared-home authority into
  AArch64 selection.
- Locate where memory/address constraints are currently rejected, flattened, or
  missing structured data.
- Separate target-valid prepared-home gaps from pure diagnostic or test gaps.
- Decide whether a supported memory/address representative is expressible with
  current prepared-home data.

Completion check:

- `todo.md` records the concrete surfaces to change, the first supported or
  blocked representative, and the narrow proof command selected by the
  supervisor.

### Step 2: Define Memory/Address Prepared-Home Authority

Goal: make the support contract explicit in structured data and diagnostics.

Primary targets:

- Prepared-home or inline-asm operand data structures.
- Constraint classification and unsupported-reason plumbing.

Actions:

- Add or clarify structured fields needed to prove target-valid memory/address
  homes.
- Preserve enough information for selection to distinguish complete homes,
  missing homes, unsupported constraints, and invalid target address forms.
- Keep allocator-dependent cases fail-closed with explicit reasons.

Completion check:

- Selection-facing data can answer whether a memory/address operand has
  target-valid authority without consulting rendered assembly text.
- Existing non-memory inline-asm tests continue to pass under the delegated
  narrow proof.

### Step 3: Select And Print Supported Memory/Address Operands

Goal: accept memory/address operands only after Step 2 proves complete
target-valid authority.

Primary targets:

- AArch64 inline-asm selection.
- AArch64 machine operand construction and printing.

Actions:

- Route structured memory/address operands into machine nodes only when the
  prepared-home contract is satisfied.
- Print accepted operands from structured machine operands, not from parsed
  final text.
- Preserve fail-closed behavior for incomplete or invalid homes.

Completion check:

- At least one supported representative passes if the prepared-home model can
  express it.
- Nearby missing-home, unsupported-constraint, and target-invalid cases fail
  closed with explicit diagnostics.

### Step 4: Harden Tests And Regression Scope

Goal: prove the route as a backend capability rather than a narrow fixture.

Primary targets:

- Backend inline-asm tests covering memory/address support and failure modes.
- Existing supported inline-asm regression subset.

Actions:

- Add supported memory/address coverage only for structured expressible homes.
- Add fail-closed cases for missing homes, unsupported constraints, and invalid
  address forms.
- Re-run existing supported inline-asm operand, name, immediate, modifier,
  side-effect, output, clobber, and tied-home coverage selected by the
  supervisor.
- Run the supervisor-selected regression guard before closure.

Completion check:

- Supported and fail-closed tests demonstrate a general prepared-home rule.
- The regression guard passes, or `todo.md` records a precise blocker that
  requires a route rewrite instead of expectation downgrades.
