# LIR Anonymous Aggregate Layout Type Facts Runbook

Status: Active
Source Idea: ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md
Activated from: blocked Step 3 of 754; return to 754 only after this bounded
type-model handoff is accepted.

## Purpose

Provide native anonymous aggregate field-layout/type facts needed by the
selected extractvalue row without letting compatibility text become authority.

## Core Rule

Native structured field facts are authority. `LirTypeRef` rendering may mirror
an anonymous aggregate but must not be parsed to create or repair its layout.

## Read First

- `ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md`
- `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`
- closed composite type-model history and current LIR type construction/verifier seams
- direct-complex aggregate lowering plus nearby focused tests

## Non-Goals

- `LirExtractValueOp` result/use/index/result-type row validation.
- Raw-BIR, other aggregate/vector rows, broad type rewrite, lowering, MIR,
  emission, and all text-derived layout recovery.

## Ordered Steps

### Step 1 - Trace and select anonymous aggregate layout facts

Goal: identify the exact anonymous aggregate construction and verification
boundary and select the smallest checked native field-layout carrier.

Actions:

- trace the direct-complex aggregate type from construction to verifier use;
- identify required field ordering, type ownership, and malformed proof seams;
- state the exact downstream 754 handoff and keep row validation out of scope.

Completion check: one bounded native layout contract is explicit and no
compatibility-text parsing or extractvalue-row work is selected.

### Step 2 - Publish checked native anonymous field layout

Goal: preserve ordered anonymous aggregate field types as native structured
facts with compatible rendering.

Actions:

- implement only the selected construction/model and validation seams;
- reject malformed or type-incoherent field layouts at the correct boundary;
- leave named structs, arrays, and unrelated paths unchanged or fail closed.

Completion check: a consumer can obtain native anonymous field count and type
without interpreting display text.

### Step 3 - Prove and publish the 754 handoff

Goal: establish positive and malformed proof and record the exact field-layout
contract that 754 Step 3 may consume.

Actions:

- add nearby same-feature acceptance and rejection coverage;
- run a fresh build, focused subset, and supervisor-selected broader proof;
- state permitted facts and rejected forms for 754's field/index/result check.

Completion check: accepted proof supports reactivation of 754 at unchanged
Step 3 without treating this blocker as extractvalue-row validation.
