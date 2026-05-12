# Current Packet

Status: Active
Source Idea Path: ideas/open/172_type_identity_authority_audit.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Write Follow-Up Implementation Ideas

## Just Finished

Step 5 - Write Follow-Up Implementation Ideas completed as lifecycle-only
planning work. No implementation behavior, tests, `plan.md`, or the active
source idea `ideas/open/172_type_identity_authority_audit.md` were changed.

Created bounded follow-up implementation ideas for the five highest-risk
type-identity gaps from the Step 4 risk map:

1. `ideas/open/173_aggregate_layout_identity_structured_boundary.md`
   covers aggregate layout identity across the HIR -> LIR -> BIR/backend
   boundary, focusing on structured layout facts, generated metadata-rich
   paths, compatibility bridge isolation, and focused layout/backend proof.
2. `ideas/open/174_aggregate_abi_classification_structured_facts.md`
   covers aggregate ABI classification at BIR or AArch64 boundaries, focusing
   on durable size/alignment/layout/ABI facts instead of rendered signature or
   instruction type strings.
3. `ideas/open/175_hir_typespec_ref_structured_equivalence.md`
   covers one bounded HIR ordinary type-ref family, separating `TypeSpec`
   syntax payload from structured resolved type identity.
4. `ideas/open/176_lir_type_ref_structured_equality.md`
   covers `LirTypeRef` equality/comparison so structured payload such as
   `StructNameId` can reject equal rendered text in metadata-rich paths.
5. `ideas/open/177_template_record_owner_structured_identity.md`
   covers template record owner identity, replacing semantic dependence on
   rendered `SpecializationKey::canonical` with structured specialization
   identity for one bounded owner path.

Each follow-up idea includes goal/intent, why it exists, in-scope work,
out-of-scope work, acceptance criteria, and domain-specific reviewer reject
signals. The ideas intentionally avoid claiming implementation progress,
testcase-shaped fixes, unsupported expectation downgrades, or new rendered
type-name parser branches as progress.

## Suggested Next

Proceed to Step 6 - Audit Closure Check. Verify the active audit now satisfies
its acceptance criteria: the audit distinguishes syntax payload, resolved type
identity, layout identity, ABI class, and display spelling; remaining
spelling-based type authority is classified and prioritized; concrete
follow-up ideas exist for the highest-risk gaps; and no tests or expectations
were weakened.

## Watchouts

- Step 6 should decide audit closure separately from implementation readiness:
  the new follow-up ideas are open initiatives and should not be treated as
  completed implementation under this audit.
- Closure should preserve the source idea's distinction between syntax
  payload, resolved type identity, layout identity, ABI class, display spelling,
  diagnostics, and compatibility bridges.
- Do not weaken existing output, layout, ABI, HIR, LIR, or backend tests during
  closure review.

## Proof

- No tests executed; this was a lifecycle-only idea creation packet with no
  implementation changes.
- Proof is artifact-based: five new `ideas/open/` follow-up files exist with
  required durable review-contract sections and Step 5 progress is recorded in
  canonical `todo.md`.
