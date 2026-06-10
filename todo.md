Status: Active
Source Idea Path: ideas/open/152_phase_b_bir_annotation_schema_candidate_audit.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Record Rejects, Cache-Only Data, And Target-Facing Data

# Current Packet

## Just Finished

Step 3 from `plan.md`: recorded the non-schema classifications in
`docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`.
The artifact now separates accepted schema candidates from whole-struct
`Prepared*` rejects, private/cache-only data, bridge/oracle data used for
prepared-query equivalence, target-facing data that must stay outside BIR, and
blockers/deferred schema questions. It explicitly rejects copying whole
`Prepared*` structs, `Prepared*Plan` records, `Prepared*Publication` records,
`PreparedAddress`, or `PreparedValueHome` shapes into BIR.

## Suggested Next

Execute Step 4 from `plan.md`: create follow-up schema ideas only for domains
justified by the Phase A relationships and the Step 2/Step 3 classification
tables. Link each follow-up from the Phase B artifact and include reject
signals that preserve the non-schema boundaries recorded in Step 3.

## Watchouts

- Step 3 classified route boundaries but did not create follow-up idea files;
  Step 4 owns that lifecycle expansion if the supervisor delegates it.
- Function-level placement remains limited to lookup/index structures for
  cheap prepared-query-shaped access. Follow-up ideas should keep durable
  payloads on value, instruction, terminator, block, or edge annotations.
- Follow-up ideas should preserve old prepared queries as bridge/oracle data
  until BIR answers prove equivalence, and should reject whole mixed
  `Prepared*` shapes as schema inputs.

## Proof

Docs-only analysis packet. No build or test proof was run because no code
changed, per the delegated proof contract. No `test_after.log` was generated
for this docs-only slice.
