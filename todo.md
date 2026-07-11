Status: Active
Source Idea Path: ideas/open/687_structured_layout_bridge_isolation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Clarify Aggregate Layout Lookup Boundaries

# Current Packet

## Just Finished

Absorbed `review/structured_layout_step2_review.md` after the Step 2 review
trigger.

The review found no testcase-overfit, no expectation or harness downgrades, and
no public BIR, target, prepared, MIR, or ABI policy drift. It judged the
typed-operand contraction as clean Step 2 work and noted that the aggregate
layout reuse contractions remain source-idea aligned but belong under Step 3's
aggregate-layout boundary rather than continued Step 2 filing.

## Suggested Next

Proceed under `plan.md` Step 3, `Clarify Aggregate Layout Lookup Boundaries`.
The next executor packet should trace or narrow aggregate layout lookup/fallback
ownership at the adapter boundary, or the supervisor may route directly to Step
5 proof if it decides the existing Step 3-adjacent contractions are enough for
this runbook.

## Watchouts

Do not file more aggregate-layout contractions under Step 2. Keep nested
`AggregateTypeLayout` ownership adapter-private, and avoid letting the cached
layout graph become public BIR, target aggregate transport, prepared/prealloc,
MIR, byval ABI placement, initializer, memory/provenance, or call ABI policy.

## Proof

Lifecycle-only update. No build or test proof was run in this packet. The
review accepted the existing executor proof for the Step 2 implementation path:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`;
passed, 100% tests passed, 0 tests failed out of 302, proof log
`test_after.log`.
