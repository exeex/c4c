# Computed-Goto Table-Element Pointer Authority Decomposition Runbook

Status: Active
Source Idea: ideas/open/767_lir_computed_goto_table_element_pointer_authority_decomposition.md
Activated from: switched from 764 Step 1 after accepted 765/766 reduced the
five-consumer family to one passing arithmetic form and four table-element
authority failures.

## Purpose

Replace the repeatedly moving monolithic carrier route with focused,
frontend-LIR producer/result capability probes for computed-goto table elements.

## Goal

Identify the narrowest generic table-element pointer-authority seam before any
new implementation packet, without consuming 764's downstream carrier work.

## Core Rule

Each focused probe must prove one structured table-element producer/result
contract. External cases are integration evidence, never authority sources or
testcase-shaped implementation selectors.

## Read First

- `ideas/open/767_lir_computed_goto_table_element_pointer_authority_decomposition.md`
- `ideas/open/764_lir_production_computed_goto_addr_value_publication.md`
- `ideas/closed/765_lir_member_bitfield_rvalue_value_identity_publication.md`
- `ideas/closed/766_lir_ssa_indexed_gep_pointer_result_authority.md`
- direct frontend-LIR production coverage and the table-element load/result
  producers reached by the four remaining integration probes

## Non-Goals

- no `IndirBr`/`LirIndirectBrOp.addr_value` publication or verifier relaxation
- no Raw-BIR/importer, 734, backend/case probe, or 765/766 rework
- no text recovery, testcase-specific logic, synthetic bridge, or broad
  pointer/rvalue/CFG/PHI/local-object/memory/va/aggregate-vector redesign

## Execution Rules

1. Run and record the exact 1/5 versus 4/5 baseline before extracting probes.
2. Use frontend-LIR production tests because the failures precede Raw-BIR and
   backend import; the external four are integration probes only.
3. Do not select an implementation seam until both source forms have one
   focused producer/result contract and direct probe.

## Ordered Steps

### Step 1 - Establish the table-element failure-family baseline

Goal: retain the exact boundary between the resolved arithmetic route and the
remaining table-element authority family.

Actions:

- run the preserved five-case command after a fresh build
- record `comp-goto-1` as the passing arithmetic integration and classify the
  other four only by their shared missing authority stop
- retain the external cases as integration evidence; do not patch them

Completion check:

- baseline records 1 pass / 4 failures and no conclusion claims a generic
  carrier publication defect.

### Step 2 - Enumerate static-local and local table-element seams

Goal: trace only far enough to distinguish the two generic source forms and
their direct LIR producer/result candidates.

Actions:

- inventory static local pointer-table element load/results separately from
  local pointer-table element load/results
- for each, identify the structured value entering and leaving its direct LIR
  producer and the missing-result authority boundary
- do not choose a repair or broaden scope

Completion check:

- a compact source-form map names each form's producer/result contract without
  using testcase numbers as ownership labels.

### Step 3 - Extract direct frontend-LIR capability probes

Goal: turn the two source forms into direct production-path probes with one
primary contract each.

Actions:

- create or extend directly relevant frontend-LIR production tests for the
  static-local and local table-element result contracts
- assert the positive structured result and nearby malformed rejection required
  by each contract; keep external tests as integration probes
- document why frontend-LIR, rather than backend/case, owns these probes

Completion check:

- each source form has one focused frontend-LIR producer/result capability
  probe that is not a reduced copy of a named external testcase.

### Step 4 - Bind probes and select the narrowest generic seam

Goal: make an evidence-backed handoff to one implementation packet or return a
specific unresolved contract blocker.

Actions:

- bind each focused probe to its direct producer/result contract and compare
  the two maps for a shared narrow implementation owner
- select no implementation until the shared or separate generic seam is clear
- report the selected seam, focused proof, and exact return to 764 Step 1

Completion check:

- the decomposition has an executable narrow generic next owner, or a precise
  separately scoped blocker; 764 remains resumable only after that capability
  route resolves.
