# Current Packet

Status: Active
Source Idea Path: ideas/open/158_bir_comparison_condition_producer_identity.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Switch comparison and branch provenance consumers

## Just Finished

Step 5 switched the select-materialization provenance read in
`preferred_fused_compare_operand_publication_target` from the prepared fused
compare operand producer helper to `bir::find_comparison_operand_producer`.

`comparison.cpp` now identifies select producers through
`bir::ComparisonProducerKind::Select` and the existing BIR select-producer
predicate before choosing the second reserved scratch register. Register target
policy, publication callers, branch legality, prepared/BIR equivalence coverage,
and selected global-load fused-branch behavior remain on their existing paths.

## Suggested Next

Continue Step 5 with exactly one additional supervisor-selected provenance
consumer switch, or move to Step 6 acceptance validation if no further consumer
switch is desired for this idea.

## Watchouts

- The preferred fused compare operand publication target now reads BIR producer
  identity, but the remaining prepared helper call sites in `comparison.cpp`
  still belong to separate provenance consumers.
- Do not combine the next provenance read with register target policy, branch
  legality, condition-code, emission, final-record, hazard, or emitted-register
  changes.
- Keep prepared-oracle equivalence checks green while switching consumers one
  at a time.
- Do not claim progress through expectation rewrites, testcase-shaped
  shortcuts, or single-case proof that omits nearby constant and
  materialized-condition coverage.

## Proof

Delegated proof passed:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`.

`test_after.log` reports the backend subset passed, including 179 backend tests
with 0 failures.
