# LIR Aggregate and Vector Value Identity Convergence Runbook

Status: Active
Source Idea: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Activated from: the dependency-ordered successor queue after closed ideas 753
and 763; this producer/schema/verifier route precedes its future one-row 734
receiver handoff.

## Purpose

Publish native structured value, type, index, and mask authority for bounded
aggregate and vector LIR operations.  Rendered `%t` spellings remain output
only and cannot recover semantic identity.

## Core Rule

Use checked current-function structured IDs and row-specific typed facts as
authority. Do not infer result, operand, index, or mask identity from rendered
LLVM text, instruction order, or testcase naming. Keep all unselected rows
fail-closed.

## Read First

- `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`
- `ideas/closed/763_lir_composite_type_ref_model.md`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/verify.cpp`
- `src/codegen/lir/print.cpp`
- aggregate/vector lowering and existing focused LIR/frontend/backend tests
- `docs/lir_to_new_bir_remaining_coverage/first_owner_matrix.md`

## Non-Goals

- CFG/PHI predecessor identity, pointer/object or memory/VA authority.
- Raw-BIR receiver work, target lowering, MIR, or emission redesign.
- Any text-derived identity, broad aggregate/vector sweep, or expectation
  downgrade.
- Selecting a 734 receiver row before an accepted exact producer handoff.

## Ordered Steps

### Step 1 - Audit and select one aggregate/vector authority row

Goal: map the listed aggregate/vector producers, consumers, and verifier seams,
then choose the smallest native structured row with a complete authority path.

Actions:

- inspect `LirExtractValueOp`, `LirInsertValueOp`, `LirInsertElementOp`,
  `LirExtractElementOp`, and `LirShuffleVectorOp` production, verification,
  printing, and use sites;
- identify the exact result/value/type/index/mask facts available for each row
  and any required closed-763 composite type carrier;
- choose one supported row only and identify nearby positive and malformed
  authority coverage.

Completion check: the selected row and its native authoritative fields are
explicit; unsupported rows remain classified and fail-closed.

### Step 2 - Publish structured result and operand authority

Goal: make the selected producer publish current-function structured result and
use identities with exact typed facts.

Actions:

- add the minimum producer/schema representation needed for the selected row;
- preserve compatibility rendering without using it as semantic input;
- reject unknown, foreign, or type-incoherent result and operand IDs.

Completion check: selected result/use identity is independent of `%t` spelling
or rendered instruction text.

### Step 3 - Verify row-specific index or mask facts

Goal: enforce the selected aggregate/vector row's exact index, element, or mask
contract at the LIR verifier boundary.

Actions:

- validate row-specific type and index/mask coherence using structured facts;
- add focused positive and negative coverage for malformed values, types,
  indices, or masks;
- retain all unrelated aggregate/vector forms unchanged and fail-closed.

Completion check: malformed structured authority rejects and stale display text
cannot repair it.

### Step 4 - Prove and hand off the bounded row

Goal: obtain the producer-side proof required for a future one-row receiver
handoff without editing Raw BIR.

Actions:

- run a fresh build, the selected same-feature test subset, and the
  supervisor-selected broader checkpoint;
- confirm the baseline is 100% passing before source closure is considered;
- record an exact one-row handoff only if authority fields, rejected forms, and
  focused proof are accepted; otherwise retain the source as an in-scope repair
  route.

Completion check: the selected row has accepted producer/verifier evidence and
no broader Raw-BIR receipt is implied.
