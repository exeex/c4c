Status: Active
Source Idea Path: ideas/open/381_rv64_object_route_short_circuit_select_join_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Select/Join Materialization Gap

# Current Packet

## Just Finished

Lifecycle switch from idea 380 to idea 381. Idea 380 is closed because the
call-argument prior-preservation owner advanced to a distinct select/join
materialization owner.

## Suggested Next

Execute Step 1: audit the prepared BIR phi-edge move bundles and RV64 object
emission around the first short-circuit join that reads `s2` on skip edges.

## Watchouts

Do not fold this back into idea 380 unless focused evidence shows the
call-argument prior-preservation repair regressed. Keep the new route based on
prepared BIR, phi-edge move bundles, value-home, and object-route publication
facts rather than representative-specific names or addresses.

## Proof

Close decision used existing `test_before.log` and `test_after.log` for the
focused 13-test backend subset. Regression guard passed with no lost ctest
passes:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
