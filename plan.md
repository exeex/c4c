# LIR Aggregate and Vector Value Identity Convergence Runbook

Status: Active
Source Idea: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Resumed from: accepted 801 native anonymous aggregate layout/type-fact handoff;
unchanged Step 3. Steps 1--2 are accepted and must not be repeated.

## Purpose

Publish native structured value, type, index, and mask authority for bounded
aggregate and vector LIR operations. Rendered `%t` spellings remain output only
and cannot recover semantic identity.

## Core Rule

Use checked current-function structured IDs and row-specific typed facts as
authority. For the selected direct-complex extract, consume 801's native
anonymous `{ float, float }` layout, never compatibility text, instruction
order, or testcase naming.

## Read First

- `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`
- `ideas/closed/801_lir_anonymous_aggregate_layout_type_facts.md`
- `ideas/closed/798_lir_operand_provenance_authority_publication.md`
- `ideas/closed/803_lir_aggregate_ssa_producer_authority_publication.md`
- `src/codegen/lir/ir.hpp`, `src/codegen/lir/verify.cpp`, and aggregate
  lowering plus nearby focused tests

## Non-Goals

- Repeating Steps 1--2, reopening 798/803/801, or generalizing to other
  aggregate/vector rows.
- Raw-BIR, CFG/PHI, target lowering, MIR, emission, or any text-derived
  identity/layout recovery.
- Weakening existing result/use/layout contracts or accepting named-case-only
  coverage as semantic progress.

## Ordered Steps

### Step 1 - Audit and select one aggregate/vector authority row — complete

Accepted in `d8e5ed3a8`: select only `LirExtractValueOp`; retain all other
aggregate/vector rows fail-closed. Do not repeat this audit.

### Step 2 - Publish structured result and operand authority — complete

Accepted in `da07100d0` with the recorded prerequisite returns. The selected
direct-complex extract publishes/consumes checked native result and aggregate
authority. Do not reopen this step while performing Step 3.

### Step 3 - Verify row-specific index or mask facts

Goal: validate only the selected `LirExtractValueOp` aggregate field/index and
result-element coherence at the verifier boundary.

Actions:

- consume 801's checked native anonymous ordered field facts for the selected
  direct-complex `{ float, float }` carrier;
- reject malformed values, out-of-range or incoherent indices, and
  field/result-type conflicts without parsing rendered type or instruction
  text;
- add nearby positive and negative coverage; preserve unrelated rows and
  existing result/use authority contracts unchanged.

Completion check: the selected row rejects malformed structured facts and
stale display text cannot repair field/index/result coherence. This is 754
work, not a re-acceptance of 801's prerequisite.

### Step 4 - Prove and hand off the bounded row

Goal: obtain the producer-side proof required for a future one-row receiver
handoff without editing Raw BIR.

Actions:

- run a fresh build, selected same-feature subset, and supervisor-selected
  broader checkpoint;
- require a 100% passing full baseline before source closure;
- record one-row authority fields, rejected forms, and focused proof only if
  accepted.

Completion check: accepted producer/verifier evidence exists with no broader
Raw-BIR receipt implied.
