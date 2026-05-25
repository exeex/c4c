Status: Active
Source Idea Path: ideas/open/06_prepared_call_preservation_source_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Prepared Prior-Preservation Lookup Contract

# Current Packet

## Just Finished

Completed Step 2 of `plan.md`: added a shared
`find_unique_indexed_prior_preserved_value_source` contract that returns one
prepared prior-preservation source with its indexed prior-call position, or an
explicit `NotFound`, `Ambiguous`, or `InvalidPreservation` rejection before
AArch64 emission.

`src/backend/prealloc/call_plans.cpp` now routes call-argument
`PriorPreservation` source selection through the shared indexed lookup instead
of the local prior-call scan, and publishes the selected preserved call
position directly from the lookup entry.

`backend_prepared_lookup_helper_test` now covers same-block and cross-block
prior-preservation lookup behavior plus no-source, ambiguous-source, and
incomplete-source rejection statuses.

## Suggested Next

Execute Step 3: publish any additional explicit source-selection fields needed
so AArch64 can consume prepared prior-preservation authority without falling
back to target-local source discovery.

## Watchouts

- `clang-format` is not installed in this environment; formatting was kept to
  the existing local style manually.
- The call-plan builder now constructs a prior-preservation lookup from prior
  calls already accumulated in `function_plan`; Step 3 should keep that
  source-selection authority prepared-side instead of reintroducing an
  AArch64-local scan.
- Ambiguous or incomplete prior-preservation records currently produce no
  `source_selection`; the shared helper carries the explicit rejection status
  for prepared-side callers that need to surface diagnostics.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_lookup_helper)$'`

Result: passed. The CTest subset passed 2/2 tests. Proof log:
`test_after.log`.
