# Current Packet

Status: Active
Source Idea Path: ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace and select anonymous aggregate layout facts

## Just Finished

- Lifecycle switch from 754: Steps 1 and 2 are accepted; Step 3 has no code
  or test changes because anonymous aggregate layout facts are absent.

## Suggested Next

- Step 1 only: trace the direct-complex anonymous aggregate type construction
  and select the minimum checked native ordered-field representation.

## Watchouts

- Do not parse `LirTypeRef` display text or add `LirExtractValueOp` field,
  index, result, or Raw-BIR work; 754 resumes only after this handoff.

## Proof

- Pending executor selection: fresh build, focused same-feature proof, then
  supervisor-selected broader validation.
