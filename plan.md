# Production Computed-Goto Address Authority Runbook

Status: Active
Source Idea: ideas/open/764_lir_production_computed_goto_addr_value_publication.md
Resumed from: completed 766 (`74379f4a2`), which now supplies the verified
SSA-based address-GEP pointer `LirValueId` required by this carrier repair.

## Purpose

Complete only the downstream production computed-goto carrier publication
revealed by the accepted 766 GEP-result handoff. The five affected tests remain
one consumer family and are not acceptable baseline debt.

## Goal

Publish the verified current-function pointer `LirValueId` into
`LirIndirectBrOp.addr_value` so `comp-goto-1`, `20040302-1`, `20041214-1`,
`920501-4`, and `920501-5` no longer fail at that carrier check.

## Core Rule

`addr_value` is semantic authority and may be published only from the verified,
current-function pointer address result. `addr`, labels, printer output,
rendered LLVM, and testcase names are never authority sources.

## Read First

- `ideas/open/764_lir_production_computed_goto_addr_value_publication.md`
- `ideas/closed/766_lir_ssa_indexed_gep_pointer_result_authority.md`
- `ideas/closed/765_lir_member_bitfield_rvalue_value_identity_publication.md`
- the computed-goto `IndirBrStmt` publication seam, `emit_indexed_gep`, and
  existing `LirIndirectBrOp` verifier checks

## Landed Prerequisites

- 765 accepted in `1e24e2081` supplies the `insn.f1.offset` RHS identity.
- 766 accepted in `74379f4a2` supplies the structured SSA-based GEP pointer
  result and nearby malformed-authority coverage. Its fresh build, focused
  `frontend_lir_call_type_ref` proof, non-regressive matching guard, and
  supervisor backend proof 5/5 are accepted.

## Non-Goals

- no Raw-BIR/importer changes or re-execution of 734 Step 7.24
- no GEP-contract/verifier change, partial/raw authority, display-text
  recovery, failure exclusion, expectation downgrade, or baseline exception
- no rvalue, CFG, PHI, local/object, memory/va, aggregate/vector,
  target-lowering, MIR, or emission-family redesign

## Execution Rules

1. Repair the common downstream carrier seam, not a named consumer.
2. Preserve fail-closed rejection for missing, invalid, foreign, non-pointer,
   display-mismatched, and partial/raw authority.
3. Prove the repaired family before a future supervisor full-baseline gate; no
   candidate may be accepted if it expands baseline failures.

## Ordered Steps

### Step 1 - Publish and prove production computed-goto address carrier authority

Goal: retain the accepted GEP pointer identity through the immediate
address-to-`LirIndirectBrOp` publication seam and eliminate the same missing
carrier-authority failure across all five consumers.

Primary targets:

- the direct computed-goto address-to-`LirIndirectBrOp` publication seam
- nearby carrier verifier and focused production-path coverage

Actions:

- confirm the typed GEP result from the accepted 766 overload reaches the
  `IndirBrStmt` publication path without a text or synthetic bridge
- minimally publish its valid pointer `LirValueId` into `addr_value`, retaining
  all existing malformed-authority checks
- add or extend focused production-path coverage without testcase-specific
  branching, then rerun all five preserved consumers
- report the typed-field handoff and proof for later plan-owner disposition to
  734; do not re-execute 734 Step 7.24

Completion check:

- a fresh build, focused positive/malformed proof, and all five affected
  consumers have no `LirIndirectBrOp.addr_value` missing-authority failure; a
  later supervisor full-suite candidate shows no new baseline failures before
  acceptance.
