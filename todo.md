Status: Active
Source Idea Path: ideas/open/273_prealloc_coordinator_and_fact_publishers_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory current publisher families and helper dependencies

# Current Packet

## Just Finished

Activated `ideas/open/273_prealloc_coordinator_and_fact_publishers_decomposition.md` into `plan.md` after closure of the schema-header decomposition.

## Suggested Next

Execute Step 1 by inventorying `src/backend/prealloc/prealloc.cpp` publisher families and selecting the first low-risk extraction packet.

## Watchouts

- Keep this route behavior-preserving; do not change prepared output, phase order, snapshots, or test expectations.
- Do not split `regalloc.cpp` or `prepared_printer.cpp` under this plan.
- Avoid broad private helper headers that become a second monolith.
- Prefer one fact-publisher family per packet unless dependency edges require grouping.

## Proof

Lifecycle-only activation. No build proof required.
