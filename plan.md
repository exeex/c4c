# LIR Aggregate and Vector Value Identity Convergence Runbook

Status: Active
Source Idea: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Resumed from: 801's blocked Step 2 full-baseline acceptance. Step 1 remains
accepted in `d8e5ed3a8`; the formerly accepted Step 2 is reopened only for
the aggregate-use authority defect recorded in the source resumption update.

## Purpose

Publish native structured value, type, index, and mask authority for the
bounded aggregate/vector LIR row. Rendered `%t` spellings remain output only
and cannot recover semantic identity.

## Core Rule

Use checked current-function structured IDs and row-specific typed facts as
authority. Do not infer result, operand, index, or mask identity from rendered
LLVM text, instruction order, or testcase naming. Keep unselected rows
fail-closed.

## Read First

- `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`
- `ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md` resumption record
- `ideas/closed/798_lir_operand_provenance_authority_publication.md`
- `src/codegen/lir/ir.hpp`, `src/codegen/lir/verify.cpp`, and aggregate lowering seams
- failing full-baseline evidence for aggregate SSA extractvalue uses

## Non-Goals

- Anonymous aggregate layout, field-index, or result-element validation; those
  remain 801/754 Step 3 work after this repair is accepted.
- CFG/PHI, pointer/object, memory/VA authority, Raw-BIR, target lowering, MIR,
  emission, broad aggregate/vector conversion, or display-text recovery.
- Weakening `LirExtractValueOp.agg` verification merely to restore a baseline.

## Ordered Steps

### Step 1 - Audit and select one aggregate/vector authority row — complete

Accepted in `d8e5ed3a8`: only `LirExtractValueOp` is selected. Do not repeat
this audit or widen to other aggregate/vector rows.

### Step 2 - Repair structured result and aggregate operand authority

Goal: repair the previously accepted selected-row contract so every supported
aggregate SSA `LirExtractValueOp.agg` use carries valid current-function
`LirValueId` authority without text recovery.

Actions:

- trace the aggregate SSA producer-to-`LirExtractValueOp.agg` path exposed by
  `positive_sema_ok_call_builtin_runtime_c` and direct-complex coverage;
- make the smallest schema/lowering/verifier repair that preserves checked
  result/use identity and exact aggregate type coherence;
- add nearby same-feature positive and malformed coverage for the repaired
  authority boundary; reject missing, unknown, foreign, stale, or
  type-incoherent authority rather than weakening the verifier;
- obtain a fresh build, focused aggregate/frontend/backend proof, then the
  supervisor-owned full baseline. Do not advance on a partial baseline.

Completion check: aggregate SSA uses have structured, current-function
authority independent of display spelling; focused proof passes and the full
baseline is 100% accepted by the supervisor.

### Step 3 - Verify row-specific index facts — blocked on 801 handoff

Goal: enforce `LirExtractValueOp` field-index and selected result-type
coherence using 801's native anonymous aggregate facts.

Completion check: start only after 801's Step 2/3 handoff is accepted; retain
unrelated rows fail-closed.

### Step 4 - Prove and hand off the bounded row

Goal: obtain accepted producer-side proof for the selected row without a
Raw-BIR receiver change.

Completion check: a 100% full baseline and accepted one-row handoff are
recorded; otherwise preserve an executable repair route.
