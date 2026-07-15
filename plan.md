# LIR Native Vaarg Operand/Result Seam Decomposition Runbook

Status: Active
Source Idea: ideas/open/783_lir_native_vaarg_operand_result_seam_decomposition.md
Supersedes: 782 Step 1 while its prerequisite vaarg operand/result contracts are decomposed.

## Purpose

Turn the repeated upstream movement of the vaarg first-bad fact into three
small, frontend-LIR structural seams before retrying any helper-field work.

## Goal

Accept the narrowest viable native vaarg operand/result contract for AArch64
GP, AArch64 FP/alignment, and AMD64 reg/stack chains, then return to 782.

## Core Rule

This is route-quality work, not PHI or generic-migration implementation. Do not
change PHI carrier/verification or use text as identity authority.

## Read First

- `ideas/open/783_lir_native_vaarg_operand_result_seam_decomposition.md`
- `ideas/open/782_lir_vaarg_phi_input_result_identity_publication.md`
- `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`
- `src/codegen/lir/hir_to_lir/call/vaarg.cpp`
- `src/codegen/lir/hir_to_lir/call/vaarg_amd64.cpp`
- the existing frontend-LIR test inventory selected in Step 2

## Non-Goals

- Do not modify `LirPhiOp`, PHI verification, predecessor/edge authority,
  Raw-BIR/importer, backend, target lowering, MIR, or emission.
- Do not begin 782 helper-field publication, generic migration, or broad API
  redesign before all three focused seam contracts are bound.
- Do not use names, labels, rendered output, instruction order, testcase text,
  side tables, or result-name maps as identity authority.

## Execution Rules

1. Run the baseline before altering tests or implementation.
2. Use frontend-LIR structural probes, not `tests/backend/case/`: the owned
   behavior is LIR construction/authority, not backend lowering.
3. Give every probe one primary contract and bind it to exactly one of the
   three named chains.
4. Do not implement a contract until Steps 1 through 3 establish and bind the
   complete three-chain evidence set.
5. Return to 782 only after the accepted outcome names all three contracts;
   751 remains parked until 782 completes.

## Ordered Steps

### Step 1 - Establish the focused frontend-LIR baseline

Goal: record a reproducible starting result for the blocked vaarg authority
family without treating the prior PHI-facing test as sufficient evidence.

Actions:

- inspect the existing frontend-LIR test inventory to select the narrow
  structural command and three-chain probe targets;
- run the selected build plus focused command before any change; and
- record the exact command, pass/fail result, and any baseline limitation in
  `todo.md`.

Completion check:

- a current baseline and a justified frontend-LIR probe location are recorded;
  no implementation or PHI change has occurred.

### Step 2 - Enumerate the three native vaarg structural seams

Goal: make each authority loss independently observable before implementation.

Actions:

- trace and name the AArch64 GP `gr_top` to `reg_addr` GEP-address seam;
- trace and name the AArch64 FP/alignment-helper operand seam; and
- trace and name the AMD64 register/stack helper-result seam.

Completion check:

- each chain has a source, propagation path, consumer boundary, and focused
  frontend-LIR structural probe target; no seam is inferred from presentation
  text or represented only by the original monolithic case.

### Step 3 - Bind focused probes to native operand/result contracts

Goal: turn the inventory into a complete, fail-closed three-chain contract set.

Actions:

- add or refine one focused structural probe per seam in the frontend-LIR test
  location established in Step 1;
- specify the authoritative source and required native propagation for every
  probe; and
- prove the probes distinguish missing/raw authority from valid structural
  propagation without changing PHI carrier/verification.

Completion check:

- all three probes are bound to one narrow native operand/result contract and
  the test evidence is sufficient to choose the smallest implementation seam.

### Step 4 - Accept the narrowest contract and return to 782

Goal: conclude the decomposition route with an executable parent return point.

Actions:

- compare the three contracts and select only the common minimal native
  operand/result publication required by 782;
- record accepted proof and any implementation commit references; and
- request lifecycle resumption of 782 Step 1, preserving its source record.

Completion check:

- 782 can resume its Step 1 helper-input publication with all three contracts
  named; no generic-migration or PHI-completion claim is made.
