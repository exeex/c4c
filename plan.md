# LIR Global-Initializer Label-Address Authority Runbook

Status: Active
Source Idea: ideas/open/769_lir_global_initializer_label_address_authority.md
Supersedes: 768 Step 3 while the static-storage initializer contract is absent

## Purpose

Establish the structured global/static initializer authority required for the
parked 768 static label-address probe, without touching direct/local rvalue or
computed-goto carrier routes.

## Goal

Represent, publish, and verify a global initializer label address using stable
enclosing-function and target-label identities.

## Core Rule

`blockaddress(...)` presentation text and function-ID-only initializer metadata
are insufficient authority. The contract must remain structured end to end.

## Read First

- `ideas/open/769_lir_global_initializer_label_address_authority.md`
- `ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md`
- `docs/lir_computed_goto_label_address_authority/step2_producer_map.md`
- direct `ConstInitEmitter`, global-lowering, `LirGlobal`, and verifier code

## Non-Goals

- no `LirOperand` direct-rvalue or automatic/local table work
- no `IndirBrStmt`/`LirIndirectBrOp.addr_value`, backend/case, raw-text
  recovery, or changes to 765/766/767
- no Raw-BIR/importer implementation; name a successor if evidence requires it

## Execution Rules

1. Keep the representation specific to structured global initializer elements.
2. Use a direct frontend-LIR positive proof plus nearby malformed rejection;
   external integration cases remain non-primary evidence.
3. If downstream use requires Raw-BIR/importer work, stop at the demonstrated
   boundary and create a separately scoped blocker rather than widening scope.

## Ordered Steps

### Step 1 - Map the static initializer representation boundary

Goal: identify the smallest existing global initializer model extension that
can carry enclosing-function and target-label identities.

Actions:

- inspect `ConstInitEmitter`, global lowering, `LirGlobal`, and verifier
- trace the current `blockaddress(...)` serialization and function-ID-only
  retention boundary
- select the structured element shape and record any Raw-BIR/importer boundary

Completion check:

- one generic LIR representation/publisher/verifier seam is named, with no
  direct/local or carrier scope drift.

### Step 2 - Implement structured publication and verifier validation

Goal: publish the selected element and reject malformed function/label links.

Actions:

- implement only the selected global-initializer representation, publication,
  and verifier checks
- add a focused frontend-LIR positive fixture and a nearby malformed fixture
- do not add importer, backend, or carrier behavior

Completion check:

- focused positive LIR retains both identities and malformed function or label
  identity is rejected by the verifier.

### Step 3 - Prove the contract and hand off to 768

Goal: accept the focused static initializer capability or identify the exact
downstream importer blocker.

Actions:

- run a fresh build and the direct focused positive/malformed proof
- assess whether existing downstream consumption needs Raw-BIR/importer work
- if required, create a distinct downstream blocker; otherwise record the
  accepted representation and return 768 to Step 3

Completion check:

- proof is accepted and lifecycle handoff states either 768’s exact return
  point or a separately scoped named downstream blocker.
