# LIR Operand Provenance Authority Publication Runbook

Status: Active
Source Idea: ideas/open/798_lir_operand_provenance_authority_publication.md
Activated from: blocked Step 2 of 754; resume 754 only after this bounded
producer/verifier handoff is accepted.

## Purpose

Preserve a native SSA aggregate-use identity across the minimal HIR
expression-to-LIR operand boundary needed by real `extractvalue` producers.

## Core Rule

Use an existing checked `LirValueId` as authority. Compatibility strings may
render an operand but must neither create nor repair provenance.

## Read First

- `ideas/open/798_lir_operand_provenance_authority_publication.md`
- `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`
- `src/codegen/lir/operands.hpp` and its verifier/use sites
- real HIR `extractvalue` lowering and surrounding expression APIs
- nearby backend fixtures for anonymous aggregate return/extractvalue

## Non-Goals

- Publishing `LirExtractValueOp` structured result/use fields or validating its
  aggregate index/type contract.
- Raw-BIR, other aggregate/vector rows, generic expression redesign, target
  lowering, MIR, emission, and all text-derived identity.

## Ordered Steps

### Step 1 - Trace the concrete provenance-loss boundary

Goal: identify the exact HIR expression APIs and LIR operand construction seam
where a valid aggregate SSA value becomes `std::string`.

Actions:

- map the real `extractvalue` producer path from existing SSA definition to
  `LirExtractValueOp` construction;
- select the minimum opt-in carrier and exact ownership/type checks it needs;
- identify nearby positive and malformed proof surfaces.

Completion check: the loss point and a bounded structured handoff contract are
explicit; no row-local 754 work is included.

### Step 2 - Propagate checked SSA operand provenance

Goal: preserve an existing `LirValueId` through the selected expression and
operand seams while retaining compatibility rendering.

Actions:

- implement only the selected opt-in carrier and propagation;
- enforce current-function existence and type coherence at the appropriate
  verifier/construction boundary;
- leave unrelated operand/expression paths unchanged or fail closed.

Completion check: the real `extractvalue` aggregate SSA path reaches LIR with
native provenance and cannot be repaired by display text.

### Step 3 - Prove and publish the 754 handoff

Goal: establish focused positive/negative evidence and record the exact
provenance contract that 754 Step 2 may consume.

Actions:

- add nearby same-feature acceptance and malformed-authority coverage;
- run a fresh build, focused subset, and supervisor-selected broader proof;
- state the exact carrier, validation, and rejected forms in the completion
  record for 754.

Completion check: accepted proof supports reactivation of 754 at Step 2
without treating this blocker as aggregate-row publication.
