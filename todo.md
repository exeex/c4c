# Current Packet

Status: Active
Source Idea Path: ideas/open/158_bir_comparison_condition_producer_identity.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Switch comparison and branch provenance consumers

## Just Finished

Step 5 switched the materialized-condition producer identity read in
`lower_materialized_compare_condition_branch` from the prepared materialized
condition producer helper to
`bir::find_materialized_condition_producer_identity`.

`comparison.cpp` now reads the comparison-producing `BinaryInst` and producer
instruction index for `branch_condition.condition_value` from the BIR block at
`context.bir_block->insts.size()`. Prepared branch facts, condition value-home
checks, target selection, condition suffix mapping, value publication hooks,
branch emission, final instruction records, hazards, and emitted-register state
remain on their existing paths.

Prepared/BIR equivalence coverage remains in place, and the existing
materialized compare branch regression still covers the recompute-before-branch
behavior.

## Suggested Next

Continue Step 5 with exactly one additional supervisor-selected provenance
consumer switch, or move to Step 6 acceptance validation if no further consumer
switch is desired for this idea.

## Watchouts

- The materialized-compare branch recompute now reads BIR producer identity, but
  the downstream publication and branch lowering helpers still intentionally use
  prepared/AArch64 authority.
- Do not combine the next provenance read with branch legality, condition-code,
  emission, final-record, hazard, or emitted-register changes.
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
