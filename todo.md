Status: Active
Source Idea Path: ideas/open/08_bir-address-projection-model-consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Projection Paths

# Current Packet

## Just Finished

Lifecycle activation initialized `plan.md` and this executor-compatible
`todo.md` for Step 1.

## Suggested Next

Execute Step 1 by inventorying local GEP, global/relative GEP, and provenance
scalar subobject projection paths. Record the repeated facts and first concrete
helper/result targets here before implementation.

## Watchouts

- Preserve BIR output, diagnostics, and testcase expectations.
- Keep `BirFunctionLowerer` as the stateful lowering owner.
- Prefer existing `memory_types.hpp` and `memory_helpers.hpp`; do not add a new
  header unless the active plan is explicitly revised.
- Do not turn the inventory into testcase-shaped matching.

## Proof

Lifecycle-only activation; no build or test proof required yet.
