Status: Active
Source Idea Path: ideas/open/147_comparison_prealloc_fact_owner.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inspect Comparison Fact Ownership

# Current Packet

## Just Finished

None. Lifecycle activation created this execution state for Step 1.

## Suggested Next

Start Step 1 from `plan.md`: inspect comparison fact ownership, identify the
concrete owner boundary, and list affected declarations and consumers before
moving code.

## Watchouts

- Name the comparison owner before moving declarations.
- Do not move AArch64 compare instruction selection or condition-code emission
  into shared prealloc ownership.
- Do not change branch-condition semantics.

## Proof

Not run. Lifecycle activation only.
