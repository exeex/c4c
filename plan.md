# LIR Aggregate and Vector Value Identity Convergence Runbook

Status: Active
Source Idea: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Repair checkpoint: the former four-step runbook completed only the selected
`LirExtractValueOp` row. Source closure is rejected because the source still
requires representative insertvalue, element, and shuffle-vector rows.

## Purpose

Complete the remaining representative aggregate/vector rows through one
row-local authority route at a time, preserving the accepted
`LirExtractValueOp` work.

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
- Reopening accepted anonymous aggregate layout, field-index, or
  result-element validation from Steps 1--3.
- CFG/PHI, pointer/object, memory/VA authority, Raw-BIR, target lowering, MIR,
  emission, broad aggregate/vector conversion, or display-text recovery.
- Weakening `LirExtractValueOp.agg` verification merely to restore a baseline.

## Accepted Steps

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

### Step 4 - Prove and hand off the bounded row — complete

Goal: obtain accepted producer-side proof for the selected row without a
Raw-BIR receiver change.

Completion check: a supervisor-accepted 100% full baseline and accepted
one-row handoff are recorded; otherwise preserve an executable repair route.

Accepted: fresh `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure` before/after each passed 3037/3037 (29.30s/30.72s). The
matching guard passed with `--allow-non-decreasing-passed`; root
`test_before.log` and `test_after.log` are the canonical captures.

## Repair Steps

### Step 5 - Audit and select the next remaining representative row

Goal: turn the remaining source scope into a single bounded next packet.

Actions:

- inspect only `LirInsertValueOp`, `LirInsertElementOp`, `LirExtractElementOp`,
  and `LirShuffleVectorOp` producer/use paths;
- select one row whose existing structured facts and missing authority boundary
  can be stated without rendered-text recovery;
- record the selected row, concrete seam, malformed/positive coverage matrix,
  and excluded rows in `todo.md` before implementation;
- if the missing prerequisite is generic provenance, layout publication, or
  another out-of-scope authority family, stop and create a separate blocker
  rather than widening this source route.

Completion check: exactly one remaining source row has a bounded execution
contract, while all other rows remain explicitly unselected.

### Step 6 - Implement and prove the selected remaining row

Goal: publish structured result/use identity and the selected row's exact typed
facts without extending authority to other rows.

Actions:

- use checked current-function IDs and native row-local type/index/mask facts
  for the Step 5 selection; validate every selected result and operand needed
  by that row;
- add nearby valid and malformed coverage for missing/foreign authority and
  the selected row's type/index/mask conflicts; keep display compatibility
  non-authoritative;
- run a fresh build, selected same-feature proof, and matching regression
  guard; request a full baseline when the supervisor judges the accumulated
  shared-code change requires it.

Completion check: exactly the selected remaining row has structural authority
and nearby positive/malformed proof, with all unselected rows unchanged.

### Step 7 - Reassess remaining source completion

Goal: make the next lifecycle decision after one bounded remaining-row packet.

Actions:

- if source rows remain, repair this runbook from a new one-row audit rather
  than silently widening Step 6;
- if every representative row is accepted, have the supervisor capture and
  accept a fresh 100% full baseline before requesting semantic closure.

Completion check: either a precise next one-row repair/blocker route exists or
all source acceptance criteria and closure proof are evidenced.
