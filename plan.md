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

### Step 1 - Audit and select one residual producer row — complete

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

Accepted in `68bd79809`: selected only scalar integer output-only
`LirInlineAsmOp.ordinary_results[0]`, with a native fresh value ID, typed
`Output` binding at index 0, and existing verifier ownership/tuple checks.
Any later 734 receipt may consume only those structured facts; compatibility
`result` text remains receiver scope.

### Step 2 - Publish and verify the selected authority — current

Goal: implement only the Step 1 selected producer/schema/verifier contract.

Actions:

- Trace the scalar integer output-only `LirInlineAsmOp.ordinary_results[0]`
  producer through native fresh-value assignment, the typed `Output` binding,
  and existing verifier ownership/tuple checks.
- Publish or correct only the missing structured fact needed to make this row
  authoritative. Do not use downstream compatibility `result` text; it is a
  future receiver concern.
- Add nearby positive and malformed-authority coverage for missing, foreign,
  incoherent, or non-output/index-mismatched facts while preserving every
  nonselected inline-assembly form as fail closed.
- Build and run the designated focused backend interface proof.

Completion check: the selected output-only scalar-integer row has native value
ID, typed output binding/index, and checked verifier ownership/tuple facts;
malformed alternatives reject; no receiver/importer, result-text, or other
inline-assembly family change occurs.

### Step 3 - Prove and hand off the bounded row — pending

Goal: run the selected focused proof and write the exact 734 receiver handoff.

Completion check: fresh build and focused positive/negative proof pass; the
handoff identifies one receiver row, rejected forms, and return to 734 only
for that row.
