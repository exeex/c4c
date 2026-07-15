# LIR Next Residual Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/816_lir_next_residual_authority_handoff.md

## Purpose

Create one producer-owned residual authority handoff that can later unblock a
single typed 734 receiver packet without presentation recovery.

## Core Rule

Select one row only. Native structured facts are required; rendered operands,
labels, and inline-assembly text never establish semantic authority.

## Read First

- `ideas/open/816_lir_next_residual_authority_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/open/796_lir_instruction_terminator_residual_authority_handoff.md`
- `docs/lir_to_new_bir_remaining_coverage/successor_queue.md`

## Non-Goals

- Raw-BIR receipt, a residual sweep, accepted cast-route changes, CFG/PHI,
  aggregate/vector, body-parameter, module-shadow, target/MIR, or rendering
  work.

## Steps

### Step 1 - Audit and select one residual producer row — current

Goal: identify exactly one residual instruction, terminator, or inline-assembly
row whose authority can be published without text recovery.

Actions:

- Trace candidate producer, schema, verifier, and focused-test seams.
- Record the selected structured value/edge/object/type facts, current-function
  ownership, allowed and rejected forms, and excluded candidates in `todo.md`.
- If no candidate has a bounded native authority path, stop and create a
  separately scoped blocker rather than inventing facts from text.

Completion check: one row has an explicit producer contract and exact proof
ladder, or a separately scoped blocker preserves this return point.

### Step 2 - Publish and verify the selected authority — pending

Goal: implement only the Step 1 selected producer/schema/verifier contract.

Completion check: selected facts are native and checked; malformed, missing,
foreign, and incoherent forms reject; no nonselected residual family changes.

### Step 3 - Prove and hand off the bounded row — pending

Goal: run the selected focused proof and write the exact 734 receiver handoff.

Completion check: fresh build and focused positive/negative proof pass; the
handoff identifies one receiver row, rejected forms, and return to 734 only
for that row.
