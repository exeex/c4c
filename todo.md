Status: Active
Source Idea Path: ideas/open/152_phase_b_bir_annotation_schema_candidate_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Build The Phase A Evidence Index

# Current Packet

## Just Finished

Step 1 from `plan.md`: built the Phase A evidence index at
`docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`.
The artifact now contains provenance, the required source list, and a
seven-route Phase A coverage table skeleton. Each row names its Phase A
artifact evidence, closed route evidence, established BIR relationship,
`Prepared*` surfaces to classify later, and closure boundaries to preserve.

## Suggested Next

Execute Step 2 from `plan.md`: classify the indexed route facts by BIR
annotation placement, starting with value-home/storage-adjacent facts and then
call, publication, edge, memory, comparison, and return-related facts.

## Watchouts

- This is analysis-only; do not edit implementation or schema code.
- Do not invent new Phase A normalization routes; the seven rows in the Phase B
  artifact are the allowed route set for this audit.
- Preserve traceability from schema candidates back to Phase A candidate ids,
  sections, or closure notes.
- Keep target-facing AArch64 details out of durable BIR schema. The Step 1
  artifact records route-specific reject boundaries, especially register/home,
  stack-layout, ABI, target-addressing, parallel-copy execution, scratch, and
  final-instruction fields.

## Proof

Docs-only analysis packet. No build or test proof was run because no code
changed, per the delegated proof contract. No `test_after.log` was generated
for this docs-only slice.
