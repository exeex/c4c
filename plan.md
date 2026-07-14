# LIR Direct-Branch Successor Identity Publication Runbook

Status: Active
Source Idea: ideas/open/747_lir_direct_branch_successor_identity_publication.md
Activated from: paused idea 734 Step 6.3 direct-branch successor authority gap

## Purpose

Publish the minimum producer-side structured authority needed for one direct
unconditional branch. This blocker produces a handoff; it does not import a
branch into Raw BIR.

## Goal

Carry one `LirBr` successor as the existing structural `LirBlockId`, verify
same-function ownership, and hand the exact row back to idea 734.

## Core Rule

The carried block identity is semantic authority. Target-label spelling is
presentation only and must neither create nor repair an edge.

## Read First

- `ideas/open/747_lir_direct_branch_successor_identity_publication.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` resumption record
- `docs/lir_structured_identity/carrier_contract.md`
- `docs/lir_structured_identity/authority_matrix.md`

## Non-Goals

- no Raw-BIR container, importer, verifier, or test changes
- no conditional, switch, indirect, phi, or inline-assembly authority work
- no target interpretation or label-text recovery

## Execution Rules

1. Change only the direct unconditional `LirBr` producer/verifier authority
   seam and its focused proof.
2. Preserve the existing compatibility label for display without treating it as
   a semantic fallback.
3. Reject every malformed edge before a valid LIR module reaches a consumer.
4. End with a compact handoff for idea 734; do not implement its receiver.

## Ordered Steps

### Step 1 - Publish and verify one direct `LirBr` successor ID

Goal: attach the existing structural destination `LirBlockId` to the direct
unconditional branch as the sole semantic successor authority.

Primary targets:

- LIR direct-branch construction and `LirBr` carrier
- reachable LIR verifier and focused frontend LIR identity tests

Actions:

- preserve the compatibility target label, but add the exact current-function
  structural `LirBlockId` selected at branch construction to the `LirBr`
  semantic carrier; do not derive it by parsing or looking up the spelling
- verify the ID is present, valid, and owned by the same function, and that it
  identifies the exact emitted direct branch destination
- add focused valid, missing, invalid, and cross-function branch-successor
  coverage with no text-based fallback
- record a handoff for idea 734 naming the carrier, ownership contract,
  focused proof command, malformed-neighbor rejections, and the direct
  Raw-BIR jump receiver return point

Completion check:

- a fresh build and focused producer/verifier proof show one direct `LirBr`
  successor is carried and checked by `LirBlockId` only; malformed IDs reject
  and the handoff is sufficient for idea 734 to resume.

### Step 2 - Return control to idea 734

Goal: after Step 1 acceptance, close or otherwise conclude this bounded
producer blocker through the lifecycle owner and reactivate idea 734 at its
recorded Step 6.3 direct-branch receiver return point.

Completion check:

- the accepted handoff and proof reference are durable, and idea 734 has an
  executable direct-branch receiver packet without reselecting old work.
