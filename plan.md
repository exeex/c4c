# LIR Aggregate and Vector Value Identity Convergence Runbook

Status: Active
Source Idea: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Resumed from: satisfied bounded prerequisite 803 at unchanged Step 2. Step 1
was accepted in `d8e5ed3a8` and is not to be repeated.

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
- `ideas/closed/803_lir_aggregate_ssa_producer_authority_publication.md`
- `ideas/closed/798_lir_operand_provenance_authority_publication.md`
- `ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md` resumption record
- `src/codegen/lir/ir.hpp`, `src/codegen/lir/verify.cpp`, and aggregate lowering seams

## Non-Goals

- Anonymous aggregate layout, field-index, or result-element validation; these
  remain 801/754 Step 3 work after Step 2 acceptance.
- CFG/PHI, pointer/object, memory/VA authority, Raw-BIR, target lowering, MIR,
  emission, broad aggregate/vector conversion, or display-text recovery.
- Weakening `LirExtractValueOp.agg` verification merely to restore a baseline.

## Ordered Steps

### Step 1 - Audit and select one aggregate/vector authority row — complete

Accepted in `d8e5ed3a8`: only `LirExtractValueOp` is selected. Do not repeat
this audit or widen to other aggregate/vector rows.

### Step 2 - Repair structured result and aggregate operand authority

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

### Step 3 - Verify row-specific index facts — blocked on 801 handoff

Goal: enforce `LirExtractValueOp` field-index and selected result-type
coherence using 801's native anonymous aggregate facts.

Completion check: start only after 801's required handoff is accepted; retain
unrelated rows fail-closed.

### Step 4 - Prove and hand off the bounded row

Goal: obtain accepted producer-side proof for the selected row without a
Raw-BIR receiver change.

Completion check: a 100% full baseline and accepted one-row handoff are
recorded; otherwise preserve an executable repair route.
