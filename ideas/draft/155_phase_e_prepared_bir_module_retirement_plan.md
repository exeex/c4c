# 155 Phase E PreparedBirModule retirement plan

## Goal

Plan the final retirement or demotion of `PreparedBirModule` after BIR
normalization, BIR annotation schema, prealloc cache contraction, and MIR
consumer migration have enough coverage.

This is Phase E of the BIR/prealloc thinning program. It is analysis-only and
must produce follow-up ideas for the final removal path rather than directly
removing `PreparedBirModule`.

## Direction

The endgame is to stop treating `PreparedBirModule` as a second IR. If a
prepared module-like object remains, it should be a transient pass context or
annotation builder, not the durable container consumed by MIR/codegen.

## Questions

- Which fields of `PreparedBirModule` are still durable semantic state after
  Phases A-D?
- Which fields can become BIR annotations?
- Which fields can become private pass context?
- Which MIR consumers still require a prepared wrapper, and why?
- What compatibility layer is needed while retiring the old container?

## Required Analysis

Inspect:

- `src/backend/prealloc/module.hpp`
- all `PreparedBirModule` construction and mutation sites
- all MIR/codegen consumers of prepared module/function/block/value wrappers
- prepared printer/dump tooling that may need a new annotation-oriented view
- regression logs and closure notes from Phases A-D once they exist

## Expected Output

The closure note must contain:

- a field-by-field retirement map for `PreparedBirModule`;
- a list of remaining blockers before removal;
- follow-up ideas for compatibility adapters, printer/dump migration, and final
  consumer switch;
- a final proof strategy for removing or demoting the prepared module;
- explicit criteria for when the "cut the floor out" step is safe.

## Reject Signals

- Removing `PreparedBirModule` while MIR consumers still need its facts.
- Hiding the second IR behind a renamed wrapper.
- Losing prepared printer or dump visibility needed for regression diagnosis.
- Combining the final retirement with unrelated backend feature work.
