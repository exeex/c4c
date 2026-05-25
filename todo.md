Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove Or Narrow The Selected Boundary

# Current Packet

## Just Finished

Step 2 narrowed `find_prior_preserved_value_for_value` so it consumes
`context.function.call_plan_lookups` through
`prepare::find_dominating_indexed_prior_preserved_value` and returns no prior
preserved value when prepared lookup authority is unavailable. The AArch64
fallback that walked raw `call_plans->calls` and locally recomputed prepared CFG
dominance was removed.

The now-unused AArch64 dominance helpers
`argument_source_prepared_block_index_by_label`,
`argument_source_prepared_block_successors`, and `prepared_block_dominates`
were deleted from `calls_preservation.cpp` and removed from `calls.hpp`.

## Suggested Next

Continue with the next preservation-boundary slice selected by the supervisor,
likely the remaining non-dominating
`find_prior_preserved_value_for_call_argument` fallback if that is still in
scope for this source idea.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not broaden this checkpoint into full dispatch cleanup or a whole
  calls-family rewrite.
- Do not claim helper renames, expectation rewrites, or testcase-shaped
  shortcuts as progress.
- `find_prior_preserved_value_for_call_argument` still has a non-dominating
  fallback through `prepare::find_latest_indexed_prior_preserved_value`; this
  packet intentionally left it unchanged.
- `calls_moves.cpp` consumes `make_prior_preserved_call_argument_source`, not
  the selected `find_prior_preserved_value_for_value` helper directly.

## Proof

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(aarch64_call_boundary_owner|call_boundary_effect_plan)'
```

Step 2 proof completed successfully and was written to `test_after.log`.
