Status: Active
Source Idea Path: ideas/open/152_phase_b_bir_annotation_schema_candidate_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Prepared Fields By Annotation Placement

# Current Packet

## Just Finished

Step 2 from `plan.md`: classified the indexed Phase A route facts by BIR
annotation placement in
`docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`.
The artifact now contains a candidate schema map organized by value,
instruction, terminator, block, edge, function, and module-level placement.
Every accepted candidate records Phase A traceability and a placement
rationale, and the route coverage table now records the Step 2 placement
decision for all seven Phase A routes.

## Suggested Next

Execute Step 3 from `plan.md`: record rejects, cache-only data, bridge/oracle
fields, target-facing data, and blockers in the Phase B artifact. Start from
the Step 2 placement map and make sure whole `Prepared*` structs, homes,
storage, ABI/layout, target addressing, move scheduling, scratch, and final
instruction facts remain outside durable BIR schema.

## Watchouts

- Step 2 intentionally accepts no module-level durable schema candidate; the
  Phase A facts remain value, instruction, terminator, block, edge, or
  function-index scoped.
- Function-level placement is limited to lookup/index structures for cheap
  prepared-query-shaped access. It should not become an ABI, stack, target
  profile, relocation, or lowering-policy container.
- Step 3 should separate rejects/cache/oracle data from the Step 2 accepted
  schema candidates without weakening the traceability already recorded.

## Proof

Docs-only analysis packet. No build or test proof was run because no code
changed, per the delegated proof contract. No `test_after.log` was generated
for this docs-only slice.
