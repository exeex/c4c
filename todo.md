Status: Active
Source Idea Path: ideas/open/173_aggregate_layout_identity_structured_boundary.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select And Map One Layout Boundary

# Current Packet

## Just Finished

Lifecycle activation created this execution state for `plan.md` Step 1.

## Suggested Next

Inspect aggregate layout handoff surfaces and select one bounded generated
HIR -> LIR -> BIR/backend boundary where rendered aggregate spelling still
influences layout lookup, mirror checks, offsets, copies, globals, or stack
layout.

## Watchouts

- Do not treat equal rendered struct/type spelling as sufficient identity for
  metadata-rich generated paths.
- Keep legacy/no-metadata compatibility explicit and isolated.
- Do not weaken layout/backend expectations or add testcase-shaped shortcuts.

## Proof

Lifecycle-only activation; no build proof required yet.
