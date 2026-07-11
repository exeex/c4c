# Current Packet

Status: Active
Source Idea Path: ideas/open/705_prepared_fact_boundary_from_bir_views.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate prepared publication production

## Just Finished

- None since the Step 2 route repair.

## Suggested Next

- Classify the next Step 2 publication seam by semantic origin and implement
  the evidence-applicability boundary, including the supported non-PHI
  prepared `JoinTransfer` edge-publication case.

## Watchouts

- `MissingPublication` must remain fail-closed whenever a BIR CFG-edge
  publication fact is applicable.  Do not convert it into success.
- A prepared-originated `JoinTransfer` path is not a fallback from failed named
  evidence.  Select it only after classifying the family and proving complete,
  unique prepared authority; do not synthesize a BIR PHI/publication relation.
- Do not widen into call plans, prepared lookups, target materializers, or the
  common MIR migration owned by idea 706.

## Proof

- Pending for the repaired Step 2 packet.
