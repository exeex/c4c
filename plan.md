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

### Step 2 - Publish and verify the selected authority — complete

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

Outcome: independent audit confirmed the complete committed contract; no code
patch was needed. `StmtEmitter::emit_inline_asm` produces a fresh value and
typed `Output`/index-0 binding. The schema preserves its native ID and type,
and the verifier checks the exact tuple, valid native ID, and current-function
ownership. Nearby tests reject missing, invalid, duplicate, role-, index-,
type-, and foreign-authority forms. Fresh designated proof and matching
before/after regression comparison both passed (1/1 tests).

### Step 3 - Prove and hand off the bounded row — current

Goal: preserve the fresh focused proof and write the exact bounded semantic
handoff for one later 734 receiver row.

Actions:

- Hand off only `StmtEmitter::emit_inline_asm`'s fresh scalar ordinary result
  plus its typed `Output` binding at constraint index 0; the verifier contract
  is exact tuple/native valid ID/current-function ownership.
- State that missing, invalid, duplicate, role-mismatched, index-mismatched,
  type-mismatched, and foreign forms remain rejected.
- Keep compatibility `result` text presentation-only and defer Raw-BIR receipt
  to one later 734 receiver packet; do not implement that receipt here.

Completion check: the fresh focused build/test proof is retained; the handoff
identifies the one permitted 734 receiver row, native facts, rejected forms,
and deferred return to 734. This bounded proof/handoff does not by itself
complete the source idea.
