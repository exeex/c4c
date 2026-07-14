# Production Computed-Goto Address Authority Runbook

Status: Active
Source Idea: ideas/open/764_lir_production_computed_goto_addr_value_publication.md
Resumed from: completed 767 (`403e86afd`), which now supplies the verified
static-local and local table-element pointer result required by this carrier
repair.

## Purpose

Complete only the downstream production computed-goto carrier publication
after accepted 765, 766, and 767 producer/result handoffs. The five affected
tests remain one consumer family and are not acceptable baseline debt.

## Goal

Publish the verified current-function pointer `LirValueId` into
`LirIndirectBrOp.addr_value` so `comp-goto-1`, `20040302-1`, `20041214-1`,
`920501-4`, and `920501-5` no longer fail at that carrier check.

## Core Rule

`addr_value` is semantic authority and may be published only from a verified,
current-function pointer address result. `addr`, labels, printer output,
rendered LLVM, and testcase names are never authority sources.

## Read First

- `ideas/open/764_lir_production_computed_goto_addr_value_publication.md`
- `ideas/closed/767_lir_computed_goto_table_element_pointer_authority_decomposition.md`
- `ideas/closed/766_lir_ssa_indexed_gep_pointer_result_authority.md`
- `ideas/closed/765_lir_member_bitfield_rvalue_value_identity_publication.md`
- the computed-goto `IndirBrStmt` publication seam and existing
  `LirIndirectBrOp` verifier checks

## Landed Prerequisites

- 765 accepted in `1e24e2081` supplies the `insn.f1.offset` RHS identity.
- 766 accepted in `74379f4a2` supplies the structured SSA-based GEP pointer
  result and nearby malformed-authority coverage.
- 767 accepted in `403e86afd` supplies structured static-global and
  current-function-local table-element GEP/load pointer results. Its fresh
  build, direct frontend-LIR test, aggregate `^frontend_cxx_` CTest 1/1, and
  matching allow-non-decreasing guard are accepted; it did not publish the
  downstream carrier field.

## Non-Goals

- no Raw-BIR/importer changes or re-execution of 734 Step 7.24
- no reimplementation of accepted 765/766/767 producer contracts,
  GEP-contract/verifier change, partial/raw authority, display-text recovery,
  failure exclusion, expectation downgrade, or baseline exception
- no rvalue, CFG, PHI, local/object, memory/va, aggregate/vector,
  target-lowering, MIR, or emission-family redesign

## Execution Rules

1. Rerun the exact five-consumer command before changing the carrier route;
   classify results by the shared downstream authority boundary, not testcase.
2. Repair only the common `IndirBrStmt` address-to-carrier seam if verified
   upstream authority reaches it; preserve all existing fail-closed checks.
3. The accepted 767 table-element route is a prerequisite, not implementation
   scope. Do not reopen it or use its producer-only unverified-lowering fixture
   as carrier proof.
4. Prove the repaired family before a future supervisor full-baseline gate; no
   candidate may be accepted if it expands baseline failures.

## Ordered Steps

### Step 1 - Publish and prove production computed-goto address carrier authority

Goal: retain the accepted 765/766/767 pointer identity through the immediate
address-to-`LirIndirectBrOp` publication seam and eliminate the same missing
carrier-authority failure across all five consumers.

Primary targets:

- the direct computed-goto address-to-`LirIndirectBrOp` publication seam
- nearby carrier verifier and focused production-path coverage

Actions:

- fresh-build and rerun all five preserved consumers; confirm whether the
  verified table-element pointer result now reaches `IndirBrStmt`
- if needed, minimally publish that valid current-function pointer
  `LirValueId` into `addr_value`, retaining all malformed-authority checks
- add or extend focused production-path coverage without testcase-specific
  branching, then rerun all five preserved consumers
- report the typed-field handoff and proof for plan-owner disposition to 734;
  do not re-execute 734 Step 7.24

Completion check:

- a fresh build, focused positive/malformed proof, and all five affected
  consumers have no `LirIndirectBrOp.addr_value` missing-authority failure; a
  later supervisor full-suite candidate shows no new baseline failures before
  acceptance.
