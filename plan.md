# Production Computed-Goto Address Authority Runbook

Status: Active
Source Idea: ideas/open/764_lir_production_computed_goto_addr_value_publication.md
Resumed from: 768's accepted native direct-label-address producer handoff
(`9cb82f9cb`, `628b55b9`); return at Step 1.

## Purpose

Repair only the remaining external computed-goto carrier/integration authority
route. 768 is complete as an upstream producer decomposition: the four
`addr_value` failures belong to this source, while the separately observed
`pr70460` empty-GEP-pointer failure is not carrier scope.

## Goal

Publish a verified current-function pointer `LirValueId` into
`LirIndirectBrOp.addr_value` so the four remaining computed-goto integrations
no longer stop at that carrier check.

## Core Rule

`addr_value` is semantic authority. Publish it only from a verified,
current-function pointer result; `addr`, labels, printer output, rendered LLVM,
and testcase names are never authority sources.

## Read First

- `ideas/open/764_lir_production_computed_goto_addr_value_publication.md`
- `ideas/closed/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md`
- `ideas/closed/767_lir_computed_goto_table_element_pointer_authority_decomposition.md`
- `ideas/closed/766_lir_ssa_indexed_gep_pointer_result_authority.md`
- `ideas/closed/765_lir_member_bitfield_rvalue_value_identity_publication.md`
- the computed-goto `IndirBrStmt` publication seam and existing
  `LirIndirectBrOp` verifier checks

## Landed Prerequisites

- 765 (`1e24e2081`) supplies the member/bitfield RHS identity.
- 766 (`74379f4a2`) supplies the structured SSA indexed-GEP pointer result.
- 767 (`403e86afd`) supplies structured static-global and local table-element
  GEP/load pointer results.
- 771 (`9cb82f9cb`) preserves the direct constant in automatic initializer
  stores, and 768 Step 5 (`628b55b9`) emits the native direct-label-address
  rvalue constant with focused frontend-LIR proof 7/7.

## Non-Goals

- no Raw-BIR/importer changes or re-execution of 734 Step 7.24
- no reimplementation of accepted 765/766/767/768 producer contracts,
  GEP-contract/verifier relaxation, partial/raw authority, display-text
  recovery, failure exclusion, expectation downgrade, or baseline exception
- no repair of `pr70460`'s `LirGepOp.ptr: must not be empty` failure: create a
  separately scoped open blocker before any full-suite candidate is accepted
- no rvalue, CFG, PHI, local/object, memory/va, aggregate/vector,
  target-lowering, MIR, or emission-family redesign

## Execution Rules

1. Rerun the preserved five computed-goto consumers first; classify the four
   failures by their shared `LirIndirectBrOp.addr_value` boundary, not testcase.
2. Repair only the first evidenced authority-loss seam that prevents the
   verified upstream pointer result from reaching `IndirBrStmt`; preserve all
   fail-closed checks.
3. Treat `pr70460` as a separate blocker, not as evidence permitting a GEP or
   broad table repair in this route.
4. A future full baseline remains rejected until both the four carrier cases
   and the separately owned `pr70460` family have accepted repair routes and
   proof. Do not claim full-suite acceptance from a focused carrier result.

## Ordered Steps

### Step 1 - Publish and prove production computed-goto address carrier authority

Goal: retain the accepted upstream pointer identity through the immediate
address-to-`LirIndirectBrOp` publication route and eliminate the same
missing-carrier-authority stop in the four integrations.

Primary targets:

- the direct computed-goto address-to-`LirIndirectBrOp` publication seam
- the first evidenced upstream authority-loss owner if the carrier still
  correctly copies an empty operand ID
- nearby carrier verifier and focused production-path coverage

Actions:

- fresh-build and rerun the preserved five consumers; confirm the passing
  `comp-goto-1` control and the four missing-`addr_value` failures
- trace the first authority-loss owner and, only if verified pointer authority
  reaches the statement seam, minimally publish it into `addr_value`
- add or extend focused production-path coverage without testcase-specific
  branching, then rerun the five preserved consumers
- record `pr70460` only as the separately scoped empty-GEP-pointer blocker;
  do not fix it in this source
- report the typed-field handoff and accepted proof for plan-owner disposition
  to 734; do not re-execute 734 Step 7.24

Completion check:

- a fresh build, focused positive/malformed proof, and all five preserved
  consumers have no `LirIndirectBrOp.addr_value` missing-authority failure;
  a separately scoped `pr70460` route exists before another full-suite
  candidate is considered.
