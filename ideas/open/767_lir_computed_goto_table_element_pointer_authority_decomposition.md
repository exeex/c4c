# LIR Computed-Goto Table-Element Pointer Authority Decomposition

Status: Open (active blocker for
`ideas/open/764_lir_production_computed_goto_addr_value_publication.md`)
Type: decomposition of production LIR table-element pointer authority seams
Predecessor: 764 Step 1, paused after accepted 765 and 766 prerequisites

## Goal

Decompose the remaining computed-goto label-address table-element pointer
authority family into direct frontend-LIR capability probes and one narrow,
generic producer/result contract before any further implementation is chosen.

## Why This Exists

764 Step 1 repeatedly moved its first bad fact upstream: 765 made the
member/bitfield RHS identity available, and 766 made the SSA indexed-GEP
pointer result available. A fresh build followed by the exact preserved
five-case command now passes only the pointer-arithmetic integration
`comp-goto-1`; the other four integrations still fail at the same missing
`LirIndirectBrOp.addr_value` authority. The statement seam already copies
`addr.value_id()`, proving this is not a generic carrier-publication defect.

The remaining source family is separable: static local pointer-table element
forms and a local pointer-table element form both need a structured load/result
identity. The external tests are integration probes only. Because the failure
is before Raw-BIR/backend import, direct frontend-LIR production tests are the
correct ownership surface for extracting and proving the capabilities.

## In Scope

- Establish and retain the exact four-failure baseline after the one passing
  pointer-arithmetic integration, using the preserved five-case command.
- Enumerate the generic table-element source forms: static local pointer-table
  element load/result and local pointer-table element load/result, including
  their direct LIR producer and structured result contract candidates.
- Create or extend directly relevant frontend-LIR production capability probes,
  one primary contract per probe; retain the four external cases only as
  integration probes.
- Bind every focused probe to the direct LIR producer/result contract it
  exercises, then select the narrowest generic implementation seam only after
  that mapping is evidence-backed.

## Out Of Scope

- Publishing `LirIndirectBrOp.addr_value`, changing `IndirBrStmt`, or claiming
  the four integrations repaired; those remain 764 after this decomposition.
- Verifier relaxation, partial/raw/text-derived authority, testcase-name
  branching, expectation downgrade, or a synthetic identity bridge.
- Raw-BIR/importer work, 734 Step 7.24, backend/case ownership probes, or
  redoing accepted 765 member/bitfield and 766 SSA-GEP result work.
- Broad pointer, rvalue, CFG, PHI, local/object, memory/va,
  aggregate/vector, target-lowering, MIR, or emission-family redesign.

## Acceptance Criteria

- The exact five-case command is recorded as 1 pass / 4 failures, with all
  four failures classified at the same missing carrier authority and no
  testcase-specific conclusion substituted for the source-form analysis.
- The static-local and local table-element forms each have a direct
  frontend-LIR production probe and an explicit producer/result authority
  contract; the probes are capability-oriented, not copies of external cases.
- The four external integrations remain explicitly designated as integration
  proof. The source-form mapping selects one narrow generic implementation
  seam or records an evidence-backed reason it cannot yet do so.
- The handoff names the selected seam, focused proof, and exact return action:
  resume 764 Step 1 to rerun all five consumers and publish carrier authority
  only as necessary, then return to 734 for plan-owner disposition.

## Reviewer Reject Signals

- Reject test-name-shaped branching, smaller copies of the same external
  monolith, or focused probes without one primary producer/result contract.
- Reject a generic `IndirBr` publication claim when the statement seam already
  copies `addr.value_id()`, or any verifier weakening and raw/partial/text
  recovery used to manufacture authority.
- Reject Raw-BIR/importer, 734, backend/case test ownership, or rework of
  accepted 765/766 presented as table-element capability progress.
- Reject choosing or implementing a broad pointer/table redesign before direct
  frontend-LIR probes bind the static-local and local source forms to their
  producer/result contracts.
