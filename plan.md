# Production Member/Bitfield Rvalue Identity Runbook

Status: Active
Source Idea: ideas/open/765_lir_member_bitfield_rvalue_value_identity_publication.md
Activated from: switched from 764 after its Step 1 established the upstream
member/bitfield rvalue authority blocker; 764 remains open and resumable.

## Purpose

Repair only the first production rvalue owner that leaves the computed-goto
pointer-add RHS without a structured current-function value identity.

## Goal

For `comp-goto-1.c`, carry a valid `LirValueId` for `insn.f1.offset` into the
RHS index of its address-pointer GEP without weakening the GEP authority rule.

## Core Rule

A GEP result may receive authority only after both base and index are valid
current-function structured values. Rendered text is never an authority source.

## Read First

- `ideas/open/765_lir_member_bitfield_rvalue_value_identity_publication.md`
- `ideas/open/764_lir_production_computed_goto_addr_value_publication.md`
- the member/bitfield rvalue producer and `emit_indexed_gep` reached by
  `tests/c/external/gcc_torture/src/comp-goto-1.c`

## Non-Goals

- no Raw-BIR/importer, 734 receiver, `IndirBrStmt`, or computed-goto carrier
  changes
- no GEP verifier relaxation, partial-authority publication, or text recovery
- no general member/bitfield/rvalue or adjacent identity-family redesign

## Execution Rules

1. Identify no more than the first actual production owner confirmed by the
   `insn.f1.offset` expression before changing code.
2. Preserve `verify_authoritative_gep` fail-closed behavior for raw, missing,
   invalid, and foreign indexes.
3. Build and run focused producer proof before the handoff; the supervisor owns
   regression logs and broader acceptance.

## Ordered Steps

### Step 1 - Publish the production member/bitfield RHS value identity

Goal: confirm the first identity-loss owner for `insn.f1.offset`, then make
only that owner emit the structured current-function `LirValueId` required by
the RHS index of `emit_indexed_gep`.

Actions:

- reproduce the preserved focused failure route and trace only through the
  member/bitfield rvalue producer until the first missing ID owner is concrete
- minimally publish the valid ID through that producer seam; do not create a
  GEP-only fallback or a parallel text identity model
- add focused positive production-seam coverage plus malformed/raw-index
  rejection coverage that proves the existing GEP verifier remains fail-closed
- record a handoff naming the owner, carried structured field, proof, and
  return action to 764 Step 1 only

Completion check:

- fresh build and focused proof establish a valid RHS `LirValueId` and retain
  malformed-index rejection; 764 can then resume Step 1 for address publication.
