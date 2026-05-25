Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select The Preservation Boundary Leak

# Current Packet

## Just Finished

Step 1 selected `find_prior_preserved_value_for_value` as the next concrete
preservation boundary leak. Its fallback path in
`calls_preservation.cpp` reconstructs prepared-call authority by walking
`call_plans->calls` and rechecking prepared CFG dominance with AArch64-local
`argument_source_prepared_block_index_by_label`,
`argument_source_prepared_block_successors`, and `prepared_block_dominates`.

Intended authority owner: existing prepared lookup ownership under
`prepare::PreparedCallPlanLookups`, specifically
`prepare::find_dominating_indexed_prior_preserved_value`. No missing prepared
fact is currently indicated; the Step 2 checkpoint should require indexed
prepared call-plan lookups for this query, remove the local fallback, and then
retire or privatize any now-unused AArch64 dominance helper declarations.

## Suggested Next

Execute Step 2 by narrowing `find_prior_preserved_value_for_value` to consume
`context.function.call_plan_lookups` through
`prepare::find_dominating_indexed_prior_preserved_value`, then remove the raw
prepared-call scan and local dominance fallback if no remaining AArch64 caller
needs those helpers.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not broaden this checkpoint into full dispatch cleanup or a whole
  calls-family rewrite.
- Do not claim helper renames, expectation rewrites, or testcase-shaped
  shortcuts as progress.
- `find_prior_preserved_value_for_call_argument` still has a non-dominating
  fallback through `prepare::find_latest_indexed_prior_preserved_value`; leave
  it for a separate packet unless Step 2 proves the two helpers must be
  narrowed together.
- `calls_moves.cpp` consumes `make_prior_preserved_call_argument_source`, not
  the selected `find_prior_preserved_value_for_value` helper directly.

## Proof

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_(aarch64_call_boundary_owner|call_boundary_effect_plan)'
```

Recorded focused proof scope for Step 2. No build or ctest was required or run
for this read-only selection packet, and `test_after.log` was not changed.
