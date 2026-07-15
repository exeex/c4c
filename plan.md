# LIR Aggregate and Vector Value Identity Convergence Runbook

Status: Active
Source Idea: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Resumed from: closed 804 PHI producer-authority blocker after its accepted
3037/3037 full baseline; unchanged Step 2. Step 1 is accepted in `d8e5ed3a8`
and must not be repeated.

## Purpose

Repair the selected `LirExtractValueOp` result/use authority route, then retain
the existing bounded aggregate/vector row sequence.

## Core Rule

Use checked current-function structured IDs and row-specific typed facts as
authority. Do not infer result, operand, index, or mask identity from rendered
LLVM text, instruction order, or testcase naming. Keep unselected rows
fail-closed.

## Read First

- `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`
- `ideas/closed/804_lir_phi_incoming_producer_authority_repair.md`
- `ideas/closed/803_lir_aggregate_ssa_producer_authority_publication.md`
- `ideas/closed/798_lir_operand_provenance_authority_publication.md`
- `ideas/closed/801_lir_anonymous_aggregate_layout_type_facts.md`
- `src/codegen/lir/ir.hpp`, `src/codegen/lir/verify.cpp`, and aggregate lowering seams

## Non-Goals

- Repeating accepted Step 1, 804, 806, 798, 803, or 801 work.
- Anonymous aggregate layout, field-index, or result-element validation; those
  remain Step 3 work after Step 2 acceptance.
- CFG/PHI, pointer/object, memory/VA authority, Raw-BIR, target lowering, MIR,
  emission, broad aggregate/vector conversion, or display-text recovery.
- Weakening `LirExtractValueOp.agg` verification merely to restore a baseline.

## Ordered Steps

### Step 1 - Audit and select one aggregate/vector authority row — complete

Accepted in `d8e5ed3a8`: only `LirExtractValueOp` is selected. Do not repeat
this audit or widen to other aggregate/vector rows.

### Step 2 - Repair structured result and aggregate operand authority — complete

Goal: repair the selected row so every supported aggregate SSA
`LirExtractValueOp.agg` use carries valid current-function `LirValueId`
authority without text recovery.

Actions:

- consume 798's checked direct-composite operand handoff and 803's checked
  local-load/terminal-insertvalue producer handoff only at the selected row;
- retain the unary lowering's fresh `LirExtractValueOp` result ID and verify
  that result structurally for missing, unknown, and foreign authority;
- verify aggregate-operand producer authority, aggregate type coherence, and
  the existing producer display mirror; reject stale display only for that
  operand mirror, not for the result definition itself;
- add nearby same-feature positive and malformed coverage without a generic
  use-to-definition display mechanism or rendered-text authority;
- obtain a fresh build, focused aggregate/frontend/backend proof, and the
  supervisor-owned 100% full baseline. Do not advance on a partial baseline.

Completion check: the selected result has a valid current-function ID, its
aggregate operand has checked producer authority independent of display
spelling, focused proof passes, and the full baseline is 100% accepted by the
supervisor.

Accepted in `33a6c21cc`: selected `LirExtractValueOp` result-authority
coverage rejects missing and cross-function result IDs without treating result
display spelling as authority. Focused proof passed 6/6; the fresh build had
no work; full CTest before/after captures each passed 3037/3037; and the
monotonic guard passed with `--allow-non-decreasing-passed` for the equal
repeat capture.

### Step 3 - Verify row-specific index facts — complete

Goal: use 801's accepted native anonymous aggregate layout facts to enforce
the selected `LirExtractValueOp` field-index bounds and result-element type
coherence.

Actions:

- consume only 801's checked ordered native field-layout/type facts for the
  selected direct-complex aggregate carrier;
- validate the selected extract index and result type against that layout;
- add nearby selected-row positive and malformed coverage without display-text
  parsing, generic layout publication, or changes to other aggregate/vector
  rows.

Completion check: selected-row malformed index/type combinations reject,
nearby valid forms pass, focused proof passes, and unrelated rows remain
fail-closed.

Accepted in `97137f39d`: bounded direct-complex coverage rejects index `-1`,
index `2`, and an `i32` result type while preserving the nearby valid form.
The existing verifier consumes 801's native anonymous-layout facts; this slice
does not publish generic layout facts or widen to other rows. A fresh focused
target build and `^frontend_lir_call_type_ref$` passed 1/1 before and after;
the matching regression guard passed with `--allow-non-decreasing-passed`.

### Step 4 - Prove and hand off the bounded row

Goal: obtain accepted producer-side proof for the selected row without a
Raw-BIR receiver change.

Completion check: a supervisor-accepted 100% full baseline and accepted
one-row handoff are recorded; otherwise preserve an executable repair route.
