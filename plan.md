# LIR Aggregate and Vector Value Identity Convergence Runbook

Status: Active
Source Idea: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Resumed from: satisfied bounded prerequisite 798 at unchanged Step 2; Step 1
was accepted in `d8e5ed3a8` and is not to be repeated.

## Purpose

Publish native structured value, type, index, and mask authority for bounded
aggregate and vector LIR operations. Rendered `%t` spellings remain output only
and cannot recover semantic identity.

## Core Rule

Use checked current-function structured IDs and row-specific typed facts as
authority. Do not infer result, operand, index, or mask identity from rendered
LLVM text, instruction order, or testcase naming. Keep all unselected rows
fail-closed.

## Read First

- `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`
- `ideas/closed/798_lir_operand_provenance_authority_publication.md`
- `ideas/closed/763_lir_composite_type_ref_model.md`
- `src/codegen/lir/ir.hpp`, `src/codegen/lir/verify.cpp`, and `src/codegen/lir/print.cpp`
- aggregate/vector lowering and existing focused LIR/frontend/backend tests

## Non-Goals

- CFG/PHI predecessor identity, pointer/object or memory/VA authority.
- Raw-BIR receiver work, target lowering, MIR, or emission redesign.
- Any text-derived identity, broad aggregate/vector sweep, or expectation downgrade.
- Reopening 798 scope: expression/operand provenance is consumed only through
  its accepted handoff contract.

## Ordered Steps

### Step 1 - Audit and select one aggregate/vector authority row — complete

Accepted in `d8e5ed3a8`: select only `LirExtractValueOp`; retain all other
aggregate/vector rows fail-closed. Do not repeat this audit.

### Step 2 - Publish structured result and operand authority

Goal: make the selected producer publish current-function structured result and
use identities with exact typed facts.

Actions:

- add the minimum `LirExtractValueOp` producer/schema representation needed;
- consume only 798's checked aggregate `LirOperand::ssa(display, LirValueId)`
  handoff for the aggregate use; do not recover authority from text;
- preserve compatibility rendering without using it as semantic input;
- reject unknown, foreign, or type-incoherent result and operand IDs.

Completion check: selected result/use identity is independent of `%t` spelling
or rendered instruction text, without adding index semantics reserved for Step 3.

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
  focused proof are accepted; otherwise retain the source as an in-scope repair route.

Completion check: the selected row has accepted producer/verifier evidence and
no broader Raw-BIR receipt is implied.
