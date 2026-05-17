Status: Active
Source Idea Path: ideas/open/272_prealloc_schema_header_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory current schema families and dependency edges

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and this executor-compatible `todo.md`
for Step 1. No implementation work has been performed.

## Suggested Next

Execute Step 1 from `plan.md`: inventory `src/backend/prealloc/prealloc.hpp`,
map declarations and inline helpers to focused header families, and record the
first extraction family plus dependency constraints here.

## Watchouts

- Keep this plan behavior-preserving.
- Do not change prepared output, public type names, or test expectations.
- Keep `prealloc.hpp` as the compatibility umbrella.
- Do not split implementation bodies as part of the schema header step.

## Proof

Lifecycle-only activation. No build proof was run.
