Status: Active
Source Idea Path: ideas/open/140_edge_copy_facade_split.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate edge-copy facade split and close readiness

# Current Packet

## Just Finished

Completed `plan.md` Step 4 validation for the split edge-copy facade.

- Reusable edge publication/source declarations and helper definitions now live
  under `publication_plans.hpp/.cpp`.
- Cached construction and aggregate wiring remain in `prepared_lookups.*`:
  `make_prepared_edge_publication_lookups`, `make_prepared_function_lookups`,
  `PreparedFunctionLookups::edge_publications`, and move-bundle lookup wiring.
- Current-block join routing remains in `prepared_lookups.*` and is documented
  as an AArch64 current-block routing convenience rather than a generic shared
  edge fact.
- Audit searches found no duplicate wrapper surface preserving the old mixed
  facade, no expectation/test changes, and no new predecessor rescans, BIR
  producer rediscovery, or local value-location scans replacing prepared facts.
- The diff stayed a facade ownership split and did not change edge publication
  semantics, parallel-copy authority, control-flow transfer construction,
  dispatch lowering, or memory lowering.

## Suggested Next

Active runbook steps are complete. Ask the plan owner to decide whether to
close the linked source idea or replace/deactivate the plan.

## Watchouts

- Plan-owner close review should compare the final diff against
  `ideas/open/140_edge_copy_facade_split.md` acceptance criteria.
- Current-block join routing remains target-facing but not target-local because
  moving it fully would require a separate implementation ownership change.

## Proof

`cmake --build --preset default` succeeded.
`ctest --test-dir build -j --output-on-failure -R '^backend_'` passed 179/179
tests into `test_after.log`.
Regression guard with `--allow-non-decreasing-passed` passed against
`test_before.log`: before 179/179, after 179/179, no new failures, no new
tests over 30 seconds. The fresh after log was rolled forward to
`test_before.log`.
