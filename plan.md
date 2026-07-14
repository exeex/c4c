# LIR SSA-Based Indexed-GEP Pointer-Result Authority Runbook

Status: Active
Source Idea: ideas/open/766_lir_ssa_indexed_gep_pointer_result_authority.md
Activated from: switched from 764 Step 1 after its investigation established
the upstream SSA-based indexed-GEP pointer-result authority prerequisite.

## Purpose

Repair only the contract and direct producer that prevent an SSA-based indexed
GEP from carrying a structured current-function pointer-result identity.

## Goal

Authorize and retain the production indexed-GEP pointer result without
weakening authority rules or performing the downstream computed-goto carrier
publication.

## Core Rule

A pointer-result ID is semantic authority. It may be produced only from the
contract's valid current-function structured GEP inputs; rendered text and
synthetic bridges are never authority sources.

## Read First

- `ideas/open/766_lir_ssa_indexed_gep_pointer_result_authority.md`
- `ideas/open/764_lir_production_computed_goto_addr_value_publication.md`
- `ideas/closed/765_lir_member_bitfield_rvalue_value_identity_publication.md`
- `src/codegen/lir/hir_to_lir/lvalue.cpp` (`emit_indexed_gep`) and the
  authoritative-GEP verifier/nearby LIR coverage

## Non-Goals

- no `LirIndirectBrOp` or `IndirBrStmt` publication, five-consumer proof, or
  computed-goto carrier change
- no Raw-BIR/importer work or 734 Step 7.24 re-execution
- no verifier weakening, partial/raw authority, text recovery, synthetic cast,
  alloca/load, phi, or select bridge
- no broad pointer/rvalue/CFG/PHI/local-object/memory/va/aggregate-vector/
  target-lowering/MIR/emission redesign

## Execution Rules

1. Change only the smallest contract and direct `emit_indexed_gep` producer
   surfaces established by the evidence.
2. Preserve fail-closed behavior for malformed, foreign, invalid, non-pointer,
   raw, and partial authority.
3. Prove a nearby same-feature positive and malformed case before the handoff;
   the supervisor owns any broader regression gate.

## Ordered Steps

### Step 1 - Define and publish SSA-based indexed-GEP pointer-result authority

Goal: make the verifier and direct `emit_indexed_gep` producer represent the
valid current-function pointer result of an SSA-based indexed GEP.

Actions:

- trace the existing GEP authority data model and specify the minimal
  structured SSA-base condition needed alongside valid current-function inputs
- implement the bounded verifier/producer contract so `emit_indexed_gep`
  retains and returns the pointer `LirValueId`, rather than raw string-only
  output
- do not add an identity bridge at the statement or carrier seam

Completion check:

- a fresh build passes and the direct production path has a structurally
  retained verifier-valid GEP pointer-result ID.

### Step 2 - Prove the contract and hand off to 764

Goal: demonstrate the nearby positive and malformed behavior, then make the
return route executable without consuming 764's carrier work.

Actions:

- add or extend direct nearby coverage for the SSA-based result and malformed
  rejection cases required by the resulting contract
- run the focused proof selected by the supervisor after a fresh build
- report the producer/result field, proof, and exact return action to the
  supervisor for lifecycle recording

Completion check:

- focused positive and malformed proof passes; the accepted handoff says to
  resume 764 Step 1 for `LirIndirectBrOp.addr_value` publication and its five
  consumers, not to alter them here.
